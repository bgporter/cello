/*
    Copyright (c) 2023 Brett g Porter
    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:
    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#include "cello_ipc.h"

namespace
{
// each end of a client connection must use this number in their
// headers. At some point it's probably worth finding a good way
// to parameterize this.
juce::uint32 CelloMagicIpcNumber { 0x000C3110 };
} // namespace

namespace cello
{

IpcClient::IpcClient (Object& objectToWatch, UpdateType updateType, Object* state)
: juce::InterprocessConnection { true, CelloMagicIpcNumber }
, juce::ValueTreeSynchroniser { objectToWatch }
, UpdateQueue { objectToWatch, nullptr }
, clientProperties { state }
, update { updateType }
{
    // verify that the update type makes basic sense
    // need to either send or receive
    jassert ((update & UpdateType::send) || (update & UpdateType::receive));
    // ...and if we are sending a full update, we also need to be sending (in general)
    jassert (
        !(update & UpdateType::sendFullSyncCallback) ||
        ((update & UpdateType::sendFullSyncCallback) && (update & UpdateType::send)));
}

IpcClient::IpcClient (Object& objectToWatch, const juce::String& hostName, int portNum,
                      int msTimeout, UpdateType updateType, Object* state)
: IpcClient (objectToWatch, updateType, state)
, host (hostName)
, port (portNum)
, timeout (msTimeout)
{
    jassert (host.isNotEmpty ());
}

IpcClient::IpcClient (Object& objectToWatch, const juce::String& pipeName, int msTimeout,
                      UpdateType updateType, Object* state)
: IpcClient (objectToWatch, updateType, state)
, pipe (pipeName)
, timeout (msTimeout)
{
    jassert (pipe.isNotEmpty ());
}

bool IpcClient::connect (ConnectOptions option = ConnectOptions::noOptions)
{
    if (host.isNotEmpty ())
    {
        // the options have no meaning for a socket connection, just try to
        // connect to the specified socket.
        return connectToSocket (host, port, timeout);
    }

    if (pipe.isNotEmpty ())
    {
        // else -- create and/or connect to a named pipe;
        switch (options)
        {
            case ConnectOptions::createOrFail:
                return createPipe (pipe, timeout, true);
            case ConnectOptions::mustExist:
                return connectToPipe (pipe, timeout);
            case ConnectOptions::createIfNeeded:
                return createPipe (pipe, timeout, false);
            case ConnectOptions::noOptions:
            default:
                jassertfalse;
                return false;
        }
    }
    // invalid state -- no host *or* pipe defined.
    jassertfalse;
    return false;
}

void IpcClient::connectionMade ()
{
    clientProperties.connected = true;
    if (update & UpdateType::fullUpdateOnConnect)
        sendFullSyncCallback ();
}

void IpcClient::connectionLost ()
{
    clientProperties.connected = false;
}

void IpcClient::messageReceived (const juce::MemoryBlock& message)
{
    if (update & UpdateType::receive)
    {
        pushUpdate (message);
        clientProperties.rxCount++;
    }
}
void IpcClient::stateChanged (const void* encodedChange, size_t encodedSize)
{
    if ((update & UpdateType::send) && (clientProperties.connected))
    {
        sendMessage ({ encodedChange, encodedSize });
        clientProperties.txCount++;
    }
}

//==============================================================================

IpcServerProperties::IpcServerProperties (const juce::String& path, Object* state)
: Object (path, state)
{
    // trigger callbacks when the status is set, whether the value changes or not.
    status.forceUpdate (true);
}

void IpcServerProperties::startServer (int portNum, const juce::String& address)
{
    if (running)
    {
        status = IpcServerStatus::alreadyRunning;
        return;
    }
    bindAddress = address;
    // setting the port number triggers a callback in the server to actually
    // start the server thread.
    portNumber = portNum;
}

void IpcServerProperties::stopServer ()
{
    if (!running)
    {
        status = IpcServerStatus::alreadyStopped;
        return;
    }
    bindAddress = "";
    // setting the port # < 0 will trigger the server to stop.
    portNumber = -1;
}

IpcServer::IpcServer (Object& sync, IpcClient::UpdateType updateType,
                      const juce::String& statePath, Object* state)
: syncObject { sync }
, update { updateType }
, serverProperties { statePath, state }
{
    // the server properties will change its portNumber member to let us
    // know that we should start or stop ourselves.
    serverProperties.portNumber.onPropertyChange = [this] (juce::Identifier id)
    {
        if (serverProperties.portNumber > 0)
            startServer (serverProperties.portNumber, serverProperties.bindAddress);
        else
            stopServer ();
    }
}

IpcServer::~IpcServer ()
{
    // if the server is running, stop it.
    stopServer ();
    // ...and delete all of the connection objects.
    connections.clear ();
}

bool IpcServer::startServer (int portNumber, const juce::String& bindAddress)
{
    if (isThreadRunning)
    {
        serverProperties.running = true;
        serverProperties.status  = IpcServerStatus::alreadyRunning;
        return true;
    }

    if (beginWaitingForSocket (portNumber, bindAddress))
    {
        serverProperties.running = true;
        serverProperties.status  = IpcServerStatus::startedOkay;
        return true;
    }

    serverProperties.running = false;
    serverProperties.status  = IpcServerStatus::errorStarting;
    return false;
}

bool IpcServer::stopServer ()
{
    if (!isThreadRunning ())
    {
        serverProperties.running = false;
        serverProperties.status  = IpcServerStatus::alreadyStopped;
        return true;
    }

    stop ();

    if (!isThreadRunning ())
    {
        serverProperties.running = false;
        serverProperties.status  = IpcServerStatus::stoppedOkay;
        return true;
    }
    serverProperties.status = IpcServerStatus::errorStopping;
    return false;
}

juce::InterprocessConnection* IpcServer::createConnectionObject ()
{
    // create a new IpcConnection object, and take over its ownership;
    // pass back a non-owning pointer to it so the base server class can
    // finish setting up the client connection.
    auto client { std::make_unique<IpcClient> (syncObject, update, &serverProperties) };
    juce::InterprocessConnection* connection { client.get () };
    connections.push_back (std::move (client));
    return connection;
}

} // namespace cello

#if RUN_UNIT_TESTS
#include "test/test_cello_ipc.inl"
#endif
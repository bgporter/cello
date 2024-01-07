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

#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

#include "cello_object.h"
#include "cello_sync.h"
#include "cello_value.h"

namespace cello
{

struct IpcClientProperties : public Object
{
    IpcClientProperties (Object* state);
    MAKE_VALUE_MEMBER (bool, connected, false);
    MAKE_VALUE_MEMBER (int, rxCount, 0);
    MAKE_VALUE_MEMBER (int, txCount, 0);
};

//==============================================================================

class IpcClient : public juce::InterprocessConnection,
                  public juce::ValueTreeSynchroniser,
                  public UpdateQueue
{
public:
    enum UpdateType
    {
        send                = 0x01,
        receive             = 0x02,
        fullUpdateOnConnect = 0x04
    };

    enum ConnectOptions
    {
        noOptions,     ///< default, used for sockets, which have no options.
        createOrFail,  ///< create the pipe, fail if we couldn't
        mustExist,     ///< pipe must already exist, fail if it doesn't
        createIfNeeded ///< If pipe exists, use it, otherwise create.
    };

    IpcClient (Object& objectToWatch, const juce::String& hostName, int portNum,
               int msTimeout, UpdateType updateType, Object* state = nullptr);
    IpcClient (Object& objectToWatch, const juce::String& pipeName, int msTimeout,
               UpdateType updateType, Object* state = nullptr);

    /**
     * @brief Attempt to make a connection to another IpcClient running
     * in another process.
     *
     * @param option Only meaningful when connecting to a named pipe.
     * @return bool True if we connected successfully.
     */
    bool connect (ConnectOptions option = ConnectOptions::noOptions);

private:
    friend class IpcServer;
    IpcClient (Object& objectToWatch, UpdateType updateType, Object* state = nullptr);

    void connectionMade () override;
    void connectionLost () override;
    /**
     * @brief When we receive a message, apply its changes to the tree that
     * we're watching.
     *
     * @param message containing new data.
     */
    void messageReceived (const juce::MemoryBlock& message) override;

    /**
     * @brief When the tree we're connected to changes, send those changes to the
     * other end (if we're connected and should be sending updates.)
     *
     * @param encodedChange
     * @param encodedSize
     */
    void stateChanged (const void* encodedChange, size_t encodedSize) override;

private:
    /// @brief An Object that we can use to connect to the rest of an application.
    IpcClientProperties clientProperties;

    /// When do we send or receive updates?
    UpdateType update;

    const juce::String host;
    const int port;
    const juce::String pipe;
    const int timeout;
};

//==============================================================================

enum IpcServerStatus
{
    unknown,        ///< server object not created yet?
    initialized,    ///< server object created and ready to go
    startedOkay,    ///< server started as requested.
    alreadyRunning, ///< attempt to start a server that's already running
    errorStarting,  ///< unable to start the server object.
    stoppedOkay,    ///< call to stop the server succeeded
    alreadyStopped, ///< attempt to stop a server that's not running
    errorStopping   ///< unable to stop running server.
};

struct IpcServerProperties : public Object
{
    IpcServerProperties (const juce::String& path, Object* state);
    /**
     * @brief Tell the server object we're controlling to start on the specified
     * port (and optionally which address)
     *
     * After calling this, the serverProperties' `status` member will be updated and
     * possibly its `running` member.
     *
     * @param portNumber
     * @param bindAddress
     */
    void startServer (int portNum, const juce::String& address = juce::String ());

    /**
     * @brief Tell the server object we're controlling to stop.
     *
     * After calling this, the properties' `status` member will be updated and
     * possibly its `running` member.
     */
    void stopServer ();

    /// @brief will be updated whenever the server is started or stopped.
    MAKE_VALUE_MEMBER (bool, running, false);

    /// @brief will be updated with status info on an attempt to start/stop
    MAKE_VALUE_MEMBER (IpcServerStatus, status, IpcServerStatus::unknown);

    /// @brief TCP port number to use (or in use, if we're running)
    MAKE_VALUE_MEMBER (int, portNumber, 0);

    /// @brief address to bind to, optional, see the JUCE docs
    MAKE_VALUE_MEMBER (juce::String, bindAddress, {});
};

//==============================================================================

class IpcServer : public juce::InterprocessConnectionServer
{
public:
    IpcServer (Object& sync, IpcClient::UpdateType updateType,
               const juce::String& statePath, Object* state = nullptr);
    ~IpcServer () override;

    /**
     * @brief Launch the thread that starts listening for incoming socket connections.
     *
     * @param portNumber    port number to listen on.
     * @param bindAddress   address to bind to, empty string listens on all addresses
     *                      for this machine.
     * @return bool         true if the server is running (or was already running).
     */
    bool startServer (int portNumber, const juce::String& bindAddress = juce::String ());

    /**
     * @brief Stop the server.
     *
     * @return bool     true if the server was stopped (or was not running)
     */
    bool stopServer ();

protected:
    /**
     * @brief When we get a connection, the base server class will call this so
     * that we can create and return an instance of the connection type that we
     * want to use. We'll maintain ownership of the object created here in the
     * server's `connections` vector.
     *
     * @return juce::InterprocessConnection* a non-owning pointer to the client
     * connection object that was created; the base server class will use that
     * pointer to complete the setup of the client connection object.
     */
    juce::InterprocessConnection* createConnectionObject () override;

private:
    /// @brief Object being replicated over the IPC link
    Object& syncObject;

    /// @brief Do we generate or receive updates? Do we send a full update on connect?
    IpcClient::UpdateType update;

    /// @brief Owning pointers to the connection objects we create
    std::vector<std::unique_ptr<juce::InterprocessConnection>> connections;

    /// @brief The Object we use to interact with the app, will have a child
    /// IpcClientProperties object for each connection made.
    IpcServerProperties serverProperties;
};

} // namespace cello

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

namespace cello
{
class UpdateSource
{
public:
    UpdateSource ()          = default;
    virtual ~UpdateSource () = default;
    /**
     * @brief If passed true, any call that sets any Value property on this
     * Object will result in a property change update callback being executed.
     * Default (false) behavior only performs this callback when the underlying
     * value is changed.
     *
     * This may also be controlled on a per-Value basis as well.
     *
     * @param shouldForceUpdates
     */
    void forceUpdate (bool shouldForceUpdate) { doForceUpdate = shouldForceUpdate; }

    /**
     * @return true if this Object should always issue property changed callbacks.
     */
    bool shouldForceUpdate () const { return doForceUpdate; }

private:
    bool doForceUpdate { false };
};

/**
 * @class ScopedForceUpdater
 * @brief RAII class to restrict the 'forceUpdate` value to one
 * scope.
 */
class ScopedForceUpdater
{
public:
    ScopedForceUpdater (UpdateSource& val)
    : value { val }
    {
        value.forceUpdate (true);
    }

    ~ScopedForceUpdater () { value.forceUpdate (false); }

private:
    UpdateSource& value;
};

using PropertyUpdateFn = std::function<void (juce::Identifier)>;

} // namespace cello

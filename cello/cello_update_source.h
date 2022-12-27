//
// Copyright (c) 2022 Brett g Porter. All Rights Reserved.
//

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
 * @class ScopedForceValueUpdater
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

} // namespace cello

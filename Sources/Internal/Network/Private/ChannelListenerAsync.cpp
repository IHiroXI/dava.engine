#include "Network/ChannelListenerAsync.h"

namespace DAVA
{
namespace Net
{
namespace ChannelListenerAsyncDetails
{
struct ChannelListenerProxy : public IChannelListener
{
    explicit ChannelListenerProxy(IChannelListener* obj)
        : obj(obj)
    {
    }

    void OnChannelOpen(IChannel* channel) override
    {
        obj->OnChannelOpen(channel);
    }
    void OnChannelClosed(IChannel* channel, const char8* message) override
    {
        obj->OnChannelClosed(channel, message);
    }
    void OnPacketReceived(IChannel* channel, const void* buffer, size_t length) override
    {
        obj->OnPacketReceived(channel, buffer, length);
    }
    void OnPacketSent(IChannel* channel, const void* buffer, size_t length) override
    {
        obj->OnPacketSent(channel, buffer, length);
    }
    void OnPacketDelivered(IChannel* channel, uint32 packetId) override
    {
        obj->OnPacketDelivered(channel, packetId);
    }

    IChannelListener* obj = nullptr;
};

} // namespace ChannelListenerAsyncDetails

ChannelListenerAsync::ChannelListenerAsync(IChannelListener* listener, NetCallbacksHolder* callbacksHolder)
    : listenerShared(new ChannelListenerAsyncDetails::ChannelListenerProxy(listener))
    , listenerWeak(listenerShared)
    , callbacksHolder(callbacksHolder)
{
}

void ChannelListenerAsync::OnChannelOpen(IChannel* channel)
{
    auto fn = MakeFunction(listenerWeak, &IChannelListener::OnChannelOpen);
    auto fnWithParams = Bind(fn, channel);
    callbacksHolder->AddCallback(fnWithParams);
}
void ChannelListenerAsync::OnChannelClosed(IChannel* channel, const char8* message)
{
    auto fn = MakeFunction(listenerWeak, &IChannelListener::OnChannelClosed);
    auto fnWithParams = Bind(fn, channel, message);
    callbacksHolder->AddCallback(fnWithParams);
}
void ChannelListenerAsync::OnPacketReceived(IChannel* channel, const void* buffer, size_t length)
{
    auto fn = MakeFunction(listenerWeak, &IChannelListener::OnPacketReceived);
    auto fnWithParams = Bind(fn, channel, buffer, length);
    callbacksHolder->AddCallback(fnWithParams);
}
void ChannelListenerAsync::OnPacketSent(IChannel* channel, const void* buffer, size_t length)
{
    auto fn = MakeFunction(listenerWeak, &IChannelListener::OnPacketSent);
    auto fnWithParams = Bind(fn, channel, buffer, length);
    callbacksHolder->AddCallback(fnWithParams);
}
void ChannelListenerAsync::OnPacketDelivered(IChannel* channel, uint32 packetId)
{
    auto fn = MakeFunction(listenerWeak, &IChannelListener::OnPacketDelivered);
    auto fnWithParams = Bind(fn, channel, packetId);
    callbacksHolder->AddCallback(fnWithParams);
}
}
}
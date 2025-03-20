#pragma once

#include <any>
#include <concepts>
#include <cstdint>
#include <functional>
#include <type_traits>

#include "core/log.h"
#include "keycodes.h"

// fwd

namespace {
//! Represents any "specifics" in the key
//! e.g. if you want a key to all KeyPressed events
constexpr size_t WILDCARD_HASH = 0;
}  // namespace
//
namespace meddl::events {

// Event categories for grouping and filtering events
enum class EventCategory : uint32_t {
   None = 0,
   Input = 1 << 0,
   Keyboard = 1 << 1,
   Mouse = 1 << 2,
   MouseButton = 1 << 3,
   Window = 1 << 4,
   Application = 1 << 5,
};

// Event types for identifying specific events
enum class EventType {
   None,
   WindowClose,
   WindowResize,
   WindowFocus,
   WindowLostFocus,
   WindowMoved,
   KeyPressed,
   KeyReleased,
   KeyTyped,
   MouseButtonPressed,
   MouseButtonReleased,
   MouseMoved,
   MouseScrolled
};

struct EventKey {
   EventType type{EventType::None};
   size_t specific{WILDCARD_HASH};

   constexpr bool operator==(const EventKey& other) const
   {
      return type == other.type && specific == other.specific;
   }
};
}  // namespace meddl::events

namespace std {
template <>
struct hash<meddl::events::EventKey> {
   size_t operator()(const meddl::events::EventKey& key) const
   {
      return std::hash<int>{}(static_cast<int>(key.type)) ^ (key.specific << 1);
   }
};
}  // namespace std

namespace meddl::events {
template <typename T>
struct EventTraits {
   static EventKey make_key(const T& event) { return {T::type, std::monostate{}}; }
};

struct WindowClose {
   static constexpr EventType type = EventType::WindowClose;
   static constexpr EventCategory category = EventCategory::Window;
};

struct WindowResize {
   static constexpr EventType type = EventType::WindowResize;
   static constexpr EventCategory category = EventCategory::Window;
   uint32_t width;
   uint32_t height;
};

struct KeyPressed {
   static constexpr EventType type = EventType::KeyPressed;
   static constexpr EventCategory category =
       static_cast<EventCategory>(static_cast<uint32_t>(EventCategory::Input) |
                                  static_cast<uint32_t>(EventCategory::Keyboard));
   Key keycode{Key::Unknown};
   KeyModifier modifiers{KeyModifier::None};
   bool repeated{false};
};

template <>
struct EventTraits<KeyPressed> {
   static EventKey make_key(const KeyPressed& event)
   {
      size_t hash = std::hash<int32_t>{}(static_cast<int32_t>(event.keycode));
      hash ^= std::hash<int32_t>{}(static_cast<int32_t>(event.modifiers)) << 1;
      if (event.repeated) {
         hash ^= std::hash<bool>{}(event.repeated) << 2;
      }
      return {.type = KeyPressed::type, .specific = hash};
   }
};

struct KeyReleased {
   static constexpr EventType type = EventType::KeyReleased;
   static constexpr EventCategory category =
       static_cast<EventCategory>(static_cast<uint32_t>(EventCategory::Input) |
                                  static_cast<uint32_t>(EventCategory::Keyboard));
   Key keycode;
   KeyModifier modifiers;
};

template <>
struct EventTraits<KeyReleased> {
   static EventKey make_key(const KeyReleased& event)
   {
      size_t hash = std::hash<int32_t>{}(static_cast<int32_t>(event.keycode));
      hash ^= std::hash<int32_t>{}(static_cast<int32_t>(event.modifiers)) << 1;
      return {.type = KeyReleased::type, .specific = hash};
   }
};

struct KeyTyped {
   static constexpr EventType type = EventType::KeyTyped;
   static constexpr EventCategory category =
       static_cast<EventCategory>(static_cast<uint32_t>(EventCategory::Input) |
                                  static_cast<uint32_t>(EventCategory::Keyboard));
   Key keycode;
   KeyModifier modifiers;
};

template <>
struct EventTraits<KeyTyped> {
   static EventKey make_key(const KeyTyped& event)
   {
      size_t hash = std::hash<int32_t>{}(static_cast<int32_t>(event.keycode));
      hash ^= std::hash<int32_t>{}(static_cast<int32_t>(event.modifiers)) << 1;
      return {.type = KeyTyped::type, .specific = hash};
   }
};

struct MouseMoved {
   static constexpr EventType type = EventType::MouseMoved;
   static constexpr EventCategory category = static_cast<EventCategory>(
       static_cast<uint32_t>(EventCategory::Input) | static_cast<uint32_t>(EventCategory::Mouse));
   float x;
   float y;
};

template <>
struct EventTraits<MouseMoved> {
   static EventKey make_key(const MouseMoved& event)
   {
      return {.type = MouseMoved::type, .specific = WILDCARD_HASH};
   }
};

struct MouseScrolled {
   static constexpr EventType type = EventType::MouseScrolled;
   static constexpr EventCategory category = static_cast<EventCategory>(
       static_cast<uint32_t>(EventCategory::Input) | static_cast<uint32_t>(EventCategory::Mouse));
   float x_offset;
   float y_offset;
};

template <>
struct EventTraits<MouseScrolled> {
   static EventKey make_key(const MouseScrolled& event)
   {
      return {.type = MouseScrolled::type, .specific = WILDCARD_HASH};
   }
};

struct MouseButtonPressed {
   static constexpr EventType type = EventType::MouseButtonPressed;
   static constexpr EventCategory category = static_cast<EventCategory>(
       static_cast<uint32_t>(EventCategory::Input) | static_cast<uint32_t>(EventCategory::Mouse) |
       static_cast<uint32_t>(EventCategory::MouseButton));
   MouseButton button;
   KeyModifier modifiers;
};

template <>
struct EventTraits<MouseButtonPressed> {
   static EventKey make_key(const MouseButtonPressed& event)
   {
      size_t hash = std::hash<int32_t>{}(static_cast<int32_t>(event.button));
      hash ^= std::hash<int32_t>{}(static_cast<int32_t>(event.modifiers)) << 1;
      return {.type = MouseButtonPressed::type, .specific = hash};
   }
};

struct MouseButtonReleased {
   static constexpr EventType type = EventType::MouseButtonReleased;
   static constexpr EventCategory category = static_cast<EventCategory>(
       static_cast<uint32_t>(EventCategory::Input) | static_cast<uint32_t>(EventCategory::Mouse) |
       static_cast<uint32_t>(EventCategory::MouseButton));
   MouseButton button;
   KeyModifier modifiers;
};

template <>
struct EventTraits<MouseButtonReleased> {
   static constexpr EventKey make_key(const MouseButtonReleased& event)
   {
      size_t hash = std::hash<int32_t>{}(static_cast<int32_t>(event.button));
      hash ^= std::hash<int32_t>{}(static_cast<int32_t>(event.modifiers)) << 1;
      return {.type = MouseButtonReleased::type, .specific = hash};
   }
};

//! TODO: performance?
class Event {
  public:
   template <typename T>
   Event(T&& data)
       : _type(T::type),
         _category(T::category),
         _data(std::forward<T>(data)),
         _key(EventTraits<std::remove_cvref_t<T>>::make_key(data))
   {
   }

   [[nodiscard]] constexpr EventType type() const { return _type; }
   [[nodiscard]] constexpr EventCategory category() const { return _category; }
   [[nodiscard]] constexpr EventKey key() const { return _key; }

   [[nodiscard]] bool is_of(EventCategory category) const
   {
      return (static_cast<uint32_t>(_category) & static_cast<uint32_t>(category)) != 0;
   }

   template <typename T>
   const T& data() const
   {
      return std::any_cast<const T&>(_data);
   }

   bool handled = false;

  private:
   EventType _type;
   EventCategory _category;
   std::any _data;
   EventKey _key;
};

//! TODO: Should callbacks always return bools?
//! - forces user to always do it
//! + users can easily control for errors inside
//! event handling, so probably?
using EventCallbackFn = std::function<bool(Event&)>;
using EventPredicateFn = std::function<bool(Event&)>;

template <typename F, typename EventT>
concept EventTypeCallback = requires(F f, const EventT& event) {
   { f(event) } -> std::convertible_to<bool>;
};

template <typename F>
concept EventCallback = requires(F f, const Event& event) {
   { f(event) } -> std::convertible_to<bool>;
};

template <typename P, typename EventT>
concept EventPredicate = requires(P p, const EventT& event) {
   { p(event) } -> std::convertible_to<bool>;
};

class EventHandler {
  public:
   //! Subscribe to a EventType, this is inherintly a wildcard
   //! on all other key members e.g. If you subscribe to KeyPress,
   //! This callback will be called on ANY keypress
   template <typename F>
   void subscribe(EventType type, F&& callback)
   {
      EventKey key = {.type = type, .specific = WILDCARD_HASH};
      auto wrapper = [callback = std::forward<F>(callback)](Event& e) -> bool {
         return callback(e);
      };
      _callbacks[key].push_back(std::move(wrapper));
   }

   //! Same as above, but includes a predicate option
   template <typename F, typename P>
   void subscribe(EventType type, F&& callback, P&& predicate)
   {
      EventKey key = {.type = type, .specific = WILDCARD_HASH};
      auto wrapper = [callback = std::forward<F>(callback),
                      predicate = std::forward<P>(predicate)](Event& e) -> bool {
         if (predicate(e)) {
            return callback(e);
         }
         return false;
      };
      _callbacks[key].push_back(std::move(wrapper));
   }

   //! Takes an event to subscribe to, this should be used
   //! for subscribing to anything specific e.g a specfic key
   //! with specific modifiers etc.
   template <typename T, typename F>
   void subscribe(const T& event, F&& callback)
   {
      using EventType = std::remove_cvref_t<T>;
      EventKey key = EventTraits<EventType>::make_key(event);
      auto wrapper = [callback = std::forward<F>(callback)](Event& e) -> bool {
         return callback(e);
      };
      _callbacks[key].push_back(std::move(wrapper));
   }

   //! Same as above, but with a predicate
   template <typename T, typename F, typename P>
   void subscribe(const T& event, F&& callback, P&& predicate)
   {
      using EventType = std::remove_cvref_t<T>;
      EventKey key = EventTraits<EventType>::make_key(event);
      auto wrapper = [callback = std::forward<F>(callback),
                      predicate = std::forward<P>(predicate)](Event& e) -> bool {
         if (predicate(e)) {
            return callback(e);
         }
         return false;
      };
      _callbacks[key].push_back(std::move(wrapper));
   }

   void dispatch(Event& event)
   {
      auto callbacks = _callbacks.find(event.key());
      if (callbacks != _callbacks.end()) {
         for (auto& callback : callbacks->second) {
            if (callback(event)) {
               event.handled = true;
            }
         }
      }

      // TODO: Clever way to handle wildcards..?
      EventKey wildcardKey{.type = event.type(), .specific = WILDCARD_HASH};
      if (wildcardKey != event.key()) {  // Only if not already tried
         callbacks = _callbacks.find(wildcardKey);
         if (callbacks != _callbacks.end()) {
            for (auto& callback : callbacks->second) {
               if (callback(event)) {
                  event.handled = true;
               }
            }
         }
      }
   }

   void clear() { _callbacks.clear(); }

  private:
   std::unordered_map<EventKey, std::vector<EventCallbackFn>> _callbacks;
};

template <typename T, typename... Args>
Event createEvent(Args&&... args)
{
   return Event(T{std::forward<Args>(args)...});
}
}  // namespace meddl::events
//
inline std::string eventTypeToString(meddl::events::EventType type)
{
   using namespace meddl::events;
   switch (type) {
      case EventType::None:
         return "None";
      case EventType::WindowClose:
         return "WindowClose";
      case EventType::WindowResize:
         return "WindowResize";
      case EventType::WindowFocus:
         return "WindowFocus";
      case EventType::WindowLostFocus:
         return "WindowLostFocus";
      case EventType::WindowMoved:
         return "WindowMoved";
      case EventType::KeyPressed:
         return "KeyPressed";
      case EventType::KeyReleased:
         return "KeyReleased";
      case EventType::KeyTyped:
         return "KeyTyped";
      case EventType::MouseButtonPressed:
         return "MouseButtonPressed";
      case EventType::MouseButtonReleased:
         return "MouseButtonReleased";
      case EventType::MouseMoved:
         return "MouseMoved";
      case EventType::MouseScrolled:
         return "MouseScrolled";
      default:
         return "Unknown";
   }
}

template <typename Char>
struct std::formatter<meddl::events::EventKey, Char> {
   constexpr auto parse(std::format_parse_context& ctx)
   {
      auto it = ctx.begin();
      if (it != ctx.end() && *it != '}') {
         throw std::format_error("Invalid format specifier for EventKey");
      }
      return it;
   }

   template <typename FormatContext>
   auto format(const meddl::events::EventKey& key, FormatContext& ctx) const
   {
      std::string typeStr = eventTypeToString(key.type);

      if (key.specific == WILDCARD_HASH) {
         return std::format_to(ctx.out(), "EventKey{{type={}, specific=WILDCARD}}", typeStr);
      }
      else {
         return std::format_to(
             ctx.out(), "EventKey{{type={}, specific={:x}}}", typeStr, key.specific);
      }
   }
};

//! TODO: format std::any? Sounds very annoying..
template <typename Char>
struct std::formatter<meddl::events::Event, Char> {
   constexpr auto parse(std::format_parse_context& ctx)
   {
      auto it = ctx.begin();
      if (it != ctx.end() && *it != '}') {
         throw std::format_error("Invalid format specifier for Event");
      }
      return it;
   }

   template <typename FormatContext>
   auto format(const meddl::events::Event& event, FormatContext& ctx) const
   {
      return std::format_to(ctx.out(),
                            "Event{{type={}, category={:x}, key={}, handled={}}}",
                            eventTypeToString(event.type()),
                            static_cast<uint32_t>(event.category()),
                            event.key(),
                            event.handled ? "true" : "false");
   }
};

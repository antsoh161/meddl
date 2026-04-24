#pragma once

#include <GLFW/glfw3.h>

#include <cstdint>
#include <format>
#include <string_view>
#include <type_traits>

namespace meddl::events {

enum class Key : int32_t {
   Unknown = GLFW_KEY_UNKNOWN,
   Space = GLFW_KEY_SPACE,
   Apostrophe = GLFW_KEY_APOSTROPHE,
   Comma = GLFW_KEY_COMMA,
   Minus = GLFW_KEY_MINUS,
   Period = GLFW_KEY_PERIOD,
   Slash = GLFW_KEY_SLASH,
   Num0 = GLFW_KEY_0,
   Num1 = GLFW_KEY_1,
   Num2 = GLFW_KEY_2,
   Num3 = GLFW_KEY_3,
   Num4 = GLFW_KEY_4,
   Num5 = GLFW_KEY_5,
   Num6 = GLFW_KEY_6,
   Num7 = GLFW_KEY_7,
   Num8 = GLFW_KEY_8,
   Num9 = GLFW_KEY_9,
   Semicolon = GLFW_KEY_SEMICOLON,
   Equal = GLFW_KEY_EQUAL,
   A = GLFW_KEY_A,
   B = GLFW_KEY_B,
   C = GLFW_KEY_C,
   D = GLFW_KEY_D,
   E = GLFW_KEY_E,
   F = GLFW_KEY_F,
   G = GLFW_KEY_G,
   H = GLFW_KEY_H,
   I = GLFW_KEY_I,
   J = GLFW_KEY_J,
   K = GLFW_KEY_K,
   L = GLFW_KEY_L,
   M = GLFW_KEY_M,
   N = GLFW_KEY_N,
   O = GLFW_KEY_O,
   P = GLFW_KEY_P,
   Q = GLFW_KEY_Q,
   R = GLFW_KEY_R,
   S = GLFW_KEY_S,
   T = GLFW_KEY_T,
   U = GLFW_KEY_U,
   V = GLFW_KEY_V,
   W = GLFW_KEY_W,
   X = GLFW_KEY_X,
   Y = GLFW_KEY_Y,
   Z = GLFW_KEY_Z,

   Escape = GLFW_KEY_ESCAPE,
   Enter = GLFW_KEY_ENTER,
   Tab = GLFW_KEY_TAB,
   Backspace = GLFW_KEY_BACKSPACE,
   Insert = GLFW_KEY_INSERT,
   Delete = GLFW_KEY_DELETE,
   Right = GLFW_KEY_RIGHT,
   Left = GLFW_KEY_LEFT,
   Down = GLFW_KEY_DOWN,
   Up = GLFW_KEY_UP,
   PageUp = GLFW_KEY_PAGE_UP,
   PageDown = GLFW_KEY_PAGE_DOWN,
   Home = GLFW_KEY_HOME,
   End = GLFW_KEY_END,
   CapsLock = GLFW_KEY_CAPS_LOCK,
   ScrollLock = GLFW_KEY_SCROLL_LOCK,
   NumLock = GLFW_KEY_NUM_LOCK,
   PrintScreen = GLFW_KEY_PRINT_SCREEN,
   Pause = GLFW_KEY_PAUSE,
   F1 = GLFW_KEY_F1,
   F2 = GLFW_KEY_F2,
   F3 = GLFW_KEY_F3,
   F4 = GLFW_KEY_F4,
   F5 = GLFW_KEY_F5,
   F6 = GLFW_KEY_F6,
   F7 = GLFW_KEY_F7,
   F8 = GLFW_KEY_F8,
   F9 = GLFW_KEY_F9,
   F10 = GLFW_KEY_F10,
   F11 = GLFW_KEY_F11,
   F12 = GLFW_KEY_F12,

   KP0 = GLFW_KEY_KP_0,
   KP1 = GLFW_KEY_KP_1,
   KP2 = GLFW_KEY_KP_2,
   KP3 = GLFW_KEY_KP_3,
   KP4 = GLFW_KEY_KP_4,
   KP5 = GLFW_KEY_KP_5,
   KP6 = GLFW_KEY_KP_6,
   KP7 = GLFW_KEY_KP_7,
   KP8 = GLFW_KEY_KP_8,
   KP9 = GLFW_KEY_KP_9,
   KPDecimal = GLFW_KEY_KP_DECIMAL,
   KPDivide = GLFW_KEY_KP_DIVIDE,
   KPMultiply = GLFW_KEY_KP_MULTIPLY,
   KPSubtract = GLFW_KEY_KP_SUBTRACT,
   KPAdd = GLFW_KEY_KP_ADD,
   KPEnter = GLFW_KEY_KP_ENTER,
   KPEqual = GLFW_KEY_KP_EQUAL,

   LeftShift = GLFW_KEY_LEFT_SHIFT,
   LeftControl = GLFW_KEY_LEFT_CONTROL,
   LeftAlt = GLFW_KEY_LEFT_ALT,
   LeftSuper = GLFW_KEY_LEFT_SUPER,
   RightShift = GLFW_KEY_RIGHT_SHIFT,
   RightControl = GLFW_KEY_RIGHT_CONTROL,
   RightAlt = GLFW_KEY_RIGHT_ALT,
   RightSuper = GLFW_KEY_RIGHT_SUPER,
   Menu = GLFW_KEY_MENU,
};

enum class MouseButton : int32_t {
   Left = GLFW_MOUSE_BUTTON_LEFT,
   Right = GLFW_MOUSE_BUTTON_RIGHT,
   Middle = GLFW_MOUSE_BUTTON_MIDDLE,
   Button4 = GLFW_MOUSE_BUTTON_4,
   Button5 = GLFW_MOUSE_BUTTON_5,
   Button6 = GLFW_MOUSE_BUTTON_6,
   Button7 = GLFW_MOUSE_BUTTON_7,
   Button8 = GLFW_MOUSE_BUTTON_8,
};

enum class KeyModifier : int32_t {
   None = 0,
   Shift = GLFW_MOD_SHIFT,
   Control = GLFW_MOD_CONTROL,
   Alt = GLFW_MOD_ALT,
   Super = GLFW_MOD_SUPER,
   CapsLock = GLFW_MOD_CAPS_LOCK,
   NumLock = GLFW_MOD_NUM_LOCK
};

constexpr KeyModifier operator|(KeyModifier a, KeyModifier b)
{
   return static_cast<KeyModifier>(static_cast<int32_t>(a) | static_cast<int32_t>(b));
}

constexpr KeyModifier operator&(KeyModifier a, KeyModifier b)
{
   return static_cast<KeyModifier>(static_cast<int32_t>(a) & static_cast<int32_t>(b));
}

constexpr bool operator==(KeyModifier a, KeyModifier b)
{
   return static_cast<int32_t>(a) == static_cast<int32_t>(b);
}

namespace detail {

inline const char* key_to_string(meddl::events::Key key)
{
   switch (key) {
      case meddl::events::Key::Space:
         return "Space";
      case meddl::events::Key::Apostrophe:
         return "Apostrophe";
      case meddl::events::Key::Comma:
         return "Comma";
      case meddl::events::Key::Minus:
         return "Minus";
      case meddl::events::Key::Period:
         return "Period";
      case meddl::events::Key::Slash:
         return "Slash";
      case meddl::events::Key::Num0:
         return "0";
      case meddl::events::Key::Num1:
         return "1";
      case meddl::events::Key::Num2:
         return "2";
      case meddl::events::Key::Num3:
         return "3";
      case meddl::events::Key::Num4:
         return "4";
      case meddl::events::Key::Num5:
         return "5";
      case meddl::events::Key::Num6:
         return "6";
      case meddl::events::Key::Num7:
         return "7";
      case meddl::events::Key::Num8:
         return "8";
      case meddl::events::Key::Num9:
         return "9";
      case meddl::events::Key::Semicolon:
         return "Semicolon";
      case meddl::events::Key::Equal:
         return "Equal";
      case meddl::events::Key::A:
         return "A";
      case meddl::events::Key::B:
         return "B";
      case meddl::events::Key::C:
         return "C";
      case meddl::events::Key::D:
         return "D";
      case meddl::events::Key::E:
         return "E";
      case meddl::events::Key::F:
         return "F";
      case meddl::events::Key::G:
         return "G";
      case meddl::events::Key::H:
         return "H";
      case meddl::events::Key::I:
         return "I";
      case meddl::events::Key::J:
         return "J";
      case meddl::events::Key::K:
         return "K";
      case meddl::events::Key::L:
         return "L";
      case meddl::events::Key::M:
         return "M";
      case meddl::events::Key::N:
         return "N";
      case meddl::events::Key::O:
         return "O";
      case meddl::events::Key::P:
         return "P";
      case meddl::events::Key::Q:
         return "Q";
      case meddl::events::Key::R:
         return "R";
      case meddl::events::Key::S:
         return "S";
      case meddl::events::Key::T:
         return "T";
      case meddl::events::Key::U:
         return "U";
      case meddl::events::Key::V:
         return "V";
      case meddl::events::Key::W:
         return "W";
      case meddl::events::Key::X:
         return "X";
      case meddl::events::Key::Y:
         return "Y";
      case meddl::events::Key::Z:
         return "Z";
      // Function keys
      case meddl::events::Key::Escape:
         return "Escape";
      case meddl::events::Key::Enter:
         return "Enter";
      case meddl::events::Key::Tab:
         return "Tab";
      // Add the rest of the keys as needed...
      default:
         return "Unknown Key";
   }
}

inline const char* mouse_button_to_string(meddl::events::MouseButton button)
{
   switch (button) {
      case meddl::events::MouseButton::Left:
         return "Left Mouse";
      case meddl::events::MouseButton::Right:
         return "Right Mouse";
      case meddl::events::MouseButton::Middle:
         return "Middle Mouse";
      case meddl::events::MouseButton::Button4:
         return "Mouse Button 4";
      case meddl::events::MouseButton::Button5:
         return "Mouse Button 5";
      case meddl::events::MouseButton::Button6:
         return "Mouse Button 6";
      case meddl::events::MouseButton::Button7:
         return "Mouse Button 7";
      case meddl::events::MouseButton::Button8:
         return "Mouse Button 8";
      default:
         return "Unknown Mouse Button";
   }
}
}  // namespace detail
}  // namespace meddl::events

template <>
struct std::formatter<meddl::events::Key> : std::formatter<std::string_view> {
   auto format(meddl::events::Key key, format_context& ctx) const
   {
      std::string result = std::string(meddl::events::detail::key_to_string(key)) + ":" +
                           std::to_string(static_cast<int>(key));
      return formatter<std::string_view>::format(result, ctx);
   }
};

template <>
struct std::formatter<meddl::events::MouseButton> : std::formatter<std::string_view> {
   auto format(meddl::events::MouseButton button, format_context& ctx) const
   {
      std::string result = std::string(meddl::events::detail::mouse_button_to_string(button)) +
                           ":" + std::to_string(static_cast<int>(button));
      return formatter<std::string_view>::format(result, ctx);
   }
};

template <>
struct std::formatter<meddl::events::KeyModifier> : std::formatter<std::string> {
   auto format(meddl::events::KeyModifier mods, format_context& ctx) const
   {
      std::string result;
      if (mods == meddl::events::KeyModifier::None)
         return formatter<std::string>::format("None", ctx);

      if ((mods & meddl::events::KeyModifier::Shift) == meddl::events::KeyModifier::Shift)
         result += "Shift+";
      if ((mods & meddl::events::KeyModifier::Control) == meddl::events::KeyModifier::Control)
         result += "Ctrl+";
      if ((mods & meddl::events::KeyModifier::Alt) == meddl::events::KeyModifier::Alt)
         result += "Alt+";
      if ((mods & meddl::events::KeyModifier::Super) == meddl::events::KeyModifier::Super)
         result += "Super+";
      if ((mods & meddl::events::KeyModifier::CapsLock) == meddl::events::KeyModifier::CapsLock)
         result += "CapsLock+";
      if ((mods & meddl::events::KeyModifier::NumLock) == meddl::events::KeyModifier::NumLock)
         result += "NumLock+";

      if (!result.empty()) result.pop_back();
      result += ":";
      result += std::to_string(static_cast<int32_t>(mods));

      return formatter<std::string>::format(result, ctx);
   }
};

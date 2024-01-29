// #include "core/window.h"
//
// #include "core/asserts.h"

// WindowConfig::WindowConfig() : width(800), height(600), fullscreen(false) {
// }
//
// WindowBuilder& WindowBuilder::configure(const WindowConfig& cfg) {
//   config = cfg;
//   return *this;
// }
//
// WindowBuilder& WindowBuilder::width(uint32_t width) {
//   config.width = width;
//   return *this;
// }
//
// WindowBuilder& WindowBuilder::height(uint32_t height) {
//   config.height = height;
//   return *this;
// }
//
// WindowBuilder& WindowBuilder::fullscreen(bool fullscreen) {
//   config.fullscreen = fullscreen;
//   return *this;
// }
//
// meddl::Window WindowBuilder::build() {
//   M_ASSERT(config.width > 200, "{} is too small width", config.width);
//   M_ASSERT(config.height > 200, "{} is too small height", config.height);
//   const std::string title = "Meddl engine";
//   meddl::Window w(config.width, config.height, title);
//   return w;
// }

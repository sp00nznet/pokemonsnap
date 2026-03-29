#ifndef __SNAP_RENDER_H__
#define __SNAP_RENDER_H__

#include <memory>
#include "ultramodern/renderer_context.hpp"

namespace RT64 {
    struct Application;
}

namespace snap {
    namespace renderer {
        class RT64Context final : public ultramodern::renderer::RendererContext {
        public:
            ~RT64Context() override;
            RT64Context(uint8_t* rdram, ultramodern::renderer::WindowHandle window_handle, bool developer_mode);

            bool valid() override { return static_cast<bool>(app); }
            bool update_config(const ultramodern::renderer::GraphicsConfig& old_config, const ultramodern::renderer::GraphicsConfig& new_config) override;
            void enable_instant_present() override;
            void send_dl(const OSTask* task) override;
            void update_screen() override;
            void shutdown() override;
            uint32_t get_display_framerate() const override;
            float get_resolution_scale() const override;

        private:
            std::unique_ptr<RT64::Application> app;
        };

        std::unique_ptr<ultramodern::renderer::RendererContext> create_render_context(
            uint8_t* rdram, ultramodern::renderer::WindowHandle window_handle, bool developer_mode);
    }
}

#endif

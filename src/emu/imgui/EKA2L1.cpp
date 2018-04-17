#include "EKA2L1.h"

#include "loader/sis.h"
#include "loader/eka2img.h"

#include "logger/logger.h"
#include "internal.h"
#include "imguirdr.h"
#include "io/imguiio.h"
#include "window.h"

#include <common/data_displayer.h>
#include <core/core_timing.h>
#include <core/core_arm.h>
#include <core/core_mem.h>

#include <disasm/disasm.h>

static void glfw_err_callback(int err, const char* des){
    std::cout << err << ": " << des << std::endl;
}

namespace eka2l1 {
    namespace imgui {
        eka2l1_inst::eka2l1_inst(int width, int height) {
            glfwSetErrorCallback(glfw_err_callback);

            bool res = eka2l1::imgui::init();

            if (!res) {
                throw std::runtime_error("Can not intialize the emulator's GLFW!");
            }

            emu_win = eka2l1::imgui::open_window("EKA2L1", width, height);
            emu_menu = eka2l1::imgui::menu(emu_win);

            eka2l1::imgui::setup_io(emu_win);

            ImGui::StyleColorsDark();

            mem_dumper = std::make_shared<imgui_mem_dumper>();
            eka2l1::data_dump_setup(mem_dumper);

            debug_logger = std::make_shared<eka2l1::imgui::logger>();
            eka2l1::log::setup_log(debug_logger);

            // Intialize core

            eka2l1::core_mem::init();
            eka2l1::core_timing::init();
            eka2l1::core_arm::init(core_arm::unicorn);
            eka2l1::disasm::init();

            LOG_INFO("EKA2L1: Experimental Symbian SIS Emulator");
        }

        eka2l1_inst::~eka2l1_inst() {
            eka2l1::imgui::destroy_window(emu_win);
            eka2l1::imgui::free_gl();

            // FBI SHUT IT DOWN

            eka2l1::core_mem::shutdown();
            eka2l1::core_timing::shutdown();
            eka2l1::core_arm::shutdown();
            eka2l1::disasm::shutdown();

            glfwTerminate();
        }

        void eka2l1_inst::run() {
            auto install_finished = loader::parse_sis("/home/dtt2502/Miscs/super_miners.sis");
            auto img = loader::load_eka2img("/home/dtt2502/Miscs/EKA2L1HW.exe");

            LOG_INFO("Dumping RAM code memory");

            eka2l1::dump_data_map("Memory", &core_mem::memory[0x70000000], 0xFFFFFFF, 0x70000000);

            while (!glfwWindowShouldClose(emu_win)) {
                glfwPollEvents();

                eka2l1::imgui::update_io(emu_win);
                eka2l1::imgui::newframe_gl(emu_win);

                emu_menu.draw();

                mem_dumper->draw();
                debug_logger->draw("EKA2L1 Logger");
                eka2l1::imgui::clear_gl(clear_color);

                ImGui::Render();
                eka2l1::imgui::draw_gl(ImGui::GetDrawData());
                glfwSwapBuffers(emu_win);
            }
        }
    }
}

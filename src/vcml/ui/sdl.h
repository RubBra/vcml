/******************************************************************************
 *                                                                            *
 * Copyright 2021 Jan Henrik Weinstock                                        *
 *                                                                            *
 * Licensed under the Apache License, Version 2.0 (the "License");            *
 * you may not use this file except in compliance with the License.           *
 * You may obtain a copy of the License at                                    *
 *                                                                            *
 *     http://www.apache.org/licenses/LICENSE-2.0                             *
 *                                                                            *
 * Unless required by applicable law or agreed to in writing, software        *
 * distributed under the License is distributed on an "AS IS" BASIS,          *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
 * See the License for the specific language governing permissions and        *
 * limitations under the License.                                             *
 *                                                                            *
 ******************************************************************************/

#ifndef VCML_UI_SDL_H
#define VCML_UI_SDL_H

#include "vcml/common/types.h"
#include "vcml/common/strings.h"
#include "vcml/common/report.h"
#include "vcml/common/systemc.h"

#include "vcml/logging/logger.h"

#include "vcml/ui/keymap.h"
#include "vcml/ui/fbmode.h"
#include "vcml/ui/display.h"

#include <SDL.h>

namespace vcml { namespace ui {

    struct sdl_client {
        display*      disp;
        SDL_Window*   window;
        SDL_Renderer* renderer;
        SDL_Texture*  texture;
        u32           window_id;
        u64           time_frame;
        u64           time_sim;
        u64           frames;

        void notify_key(u32 keysym, bool down);
        void notify_btn(SDL_MouseButtonEvent& event);
        void notify_pos(SDL_MouseMotionEvent& event);

        void init_window();
        void exit_window();
        void draw_window();
    };

    class sdl
    {
    private:
        mutex m_mutex;
        thread m_uithread;
        atomic<bool> m_running;
        vector<sdl_client> m_clients;

        sdl_client* find_by_window_id(u32 id);

        void check_clients();
        void poll_events();
        void draw_windows();

        void ui_run();

        sdl() = default;
        sdl(const sdl&) = delete;

    public:
        ~sdl();

        void register_display(display* disp);
        void unregister_display(display* disp);

        static display* create(u32 nr);
    };

    class sdl_display: public display
    {
    private:
        sdl& m_owner;

    public:
        sdl_display(u32 nr, sdl& owner);
        virtual ~sdl_display();

        virtual void init(const fbmode& mode, u8* fb) override;
        virtual void render() override;
        virtual void shutdown() override;
    };

}}

#endif

/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */



#ifdef GAMEPAD_USAGE
#include <Windows.h>
#include <Xinput.h>
#endif

#ifdef GAMEPAD_USAGE

void
intelli::utils::GamepadThread::run()
{
    XINPUT_STATE state;
    // Endlosschleife zum Abfragen des Controllers
    while (true) {
        ZeroMemory(&state, sizeof(XINPUT_STATE));
        DWORD dwResult = XInputGetState(0, &state); // Controller 0 abfragen

        if (dwResult == ERROR_SUCCESS) {
            // Prüfe, ob der A-Button gedrückt wurde
            if (state.Gamepad.wButtons & XINPUT_GAMEPAD_A) {
                emit buttonPressed("A");
                // Warten, um Mehrfach-Events bei langem Drücken zu vermeiden
                msleep(300);
            }
        }
        msleep(50); // Kurze Pause zwischen den Abfragen
    }
}

#endif

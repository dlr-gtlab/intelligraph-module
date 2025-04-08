/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "utils.h"

#ifdef GAMEPAD_USAGE
#include <Windows.h>
#endif



#ifdef GAMEPAD_USAGE
class intelli::utils::JoystickReader::Impl
{

public:
    DWORD lastButtonState = 0;

    DWORD lastXAxis = 0;
    DWORD lastYAxis = 0;
    DWORD lastZAxis = 0;
};

intelli::utils::JoystickReader::JoystickReader(QObject *parent) :
    QObject(parent),
    m_pimpl{std::make_unique<Impl>()}
{
    startTimer(200);
}

void
intelli::utils::JoystickReader::timerEvent(QTimerEvent* event)
{
    pollJoyStick();
}

intelli::utils::JoystickReader::~JoystickReader() = default;

void
intelli::utils::JoystickReader::pollJoyStick()
{
    JOYINFOEX joyInfo;
    joyInfo.dwSize = sizeof(JOYINFOEX);
    joyInfo.dwFlags = JOY_RETURNALL;

    MMRESULT result = joyGetPosEx(JOYSTICKID1, &joyInfo);

    if (result == JOYERR_NOERROR)
    {
        DWORD currentButtons = joyInfo.dwButtons;

        DWORD currentX = joyInfo.dwXpos;
        DWORD currentY = joyInfo.dwYpos;
        DWORD currentZ = joyInfo.dwZpos;

        if (currentX != m_pimpl->lastXAxis)
        {
            int newVal = int(currentX);
            int oldVal = int(m_pimpl->lastXAxis);

            double percNew =  1.0 - newVal / 65535.;
            double percOld =  1.0 - oldVal / 65535.;

            if (abs(percNew - percOld) > 0.005)
            {
                emit xAxisChange(percNew);
            }
        }
        m_pimpl->lastButtonState = currentButtons;
        m_pimpl->lastXAxis = currentX;
        m_pimpl->lastYAxis = currentY;
        m_pimpl->lastZAxis = currentZ;
    }
    else
    {
        gtWarning() << "No joystick connected";
    }
}


#endif


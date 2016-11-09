#include "GamepadController.hpp"

using namespace std;

namespace {
constexpr RJ::Time Dribble_Step_Time = 125 * 1000;
constexpr RJ::Time Kicker_Step_Time = 125 * 1000;
const float AXIS_MAX = 32768.0f;
}

GamepadController::GamepadController()
    : _controller(nullptr), _lastDribblerTime(0), _lastKickerTime(0) {
    // initialize using the SDL joystick
    if (SDL_Init(SDL_INIT_GAMECONTROLLER) != 0) {
        cerr << "ERROR: SDL could not initialize game controller system! SDL "
                "Error: " << SDL_GetError() << endl;
        return;
    }


    // Load controller mappings
    // TODO fix this path
    SDL_GameControllerAddMapping("030000006d04000016c2000011010000,Logitech Logitech Dual Action,platform:Linux,x:b0,a:b1,b:b2,y:b3,back:b8,start:b9,dpleft:h0.8,dpdown:h0.0,dpdown:h0.4,dpright:h0.0,dpright:h0.2,dpup:h0.0,dpup:h0.1,leftshoulder:h0.0,dpup:h0.1,leftshoulder:h0.0,leftshoulder:b4,lefttrigger:b6,rightshoulder:b5,righttrigger:b7,leftstick:b10,rightstick:b11,leftx:a0,lefty:a1,rightx:a2,righty:a3,");
    // int ret = SDL_GameControllerAddMappingsFromFile("/home/jay/Code/robocup-software/run/gamecontrollerdb.txt");
    // if (ret = -1) {
    //     cout << "Loading gamecontroller mappings FAILED: " << SDL_GetError() << endl;
    // } else {
    //     cout << "Added " << ret << " mappings!" << endl;
    // }

    // Controllers will be detected later if needed.
    connected = false;
    openJoystick();
}

GamepadController::~GamepadController() {
    QMutexLocker(&mutex());
    SDL_GameControllerClose(_controller);
    _controller = nullptr;
    SDL_Quit();
}

void GamepadController::openJoystick() {
    if (SDL_NumJoysticks()) {
        // Open the first available controller
        for (size_t i = 0; i < SDL_NumJoysticks(); ++i) {
            // setup the joystick as a game controller if available
            // if (SDL_IsGameController(i)) {
                SDL_GameController* controller;
                controller = SDL_GameControllerOpen(i);
                connected = true;

                if (controller != nullptr) {
                    _controller = controller;
                    cout << "Using " << SDL_GameControllerName(_controller)
                         << " game controller" << endl;
                    break;
                } else {
                    cerr << "ERROR: Could not open controller! SDL Error: "
                         << SDL_GetError() << endl;
                }
            // }
        }
    }

    // _controller = SDL_GameControllerOpen(device);
    // SDL_Joystick* j = SDL_GameControllerGetJoystick(_controller);
    // SDL_JoystickID m_instance_id = SDL_JoystickInstanceID(j);
}

void GamepadController::closeJoystick() {
    SDL_GameControllerClose(_controller);
    cout << "Gamepad Controller Disconnected" << endl;
    connected = false;
}

bool GamepadController::valid() const { return _controller != nullptr; }

void GamepadController::update() {
    QMutexLocker(&mutex());
    SDL_GameControllerUpdate();

    RJ::Time now = RJ::timestamp();

    if (connected) {
        // Check if dc
        if (!SDL_GameControllerGetAttached(_controller)) {
            closeJoystick();
            return;
        }
    } else {
        // Check if new controller found
        openJoystick();
        if (!connected) {
            return;
        }
    }

    /*
     *  DRIBBLER POWER
     */
    if (SDL_GameControllerGetButton(_controller,
                                    SDL_CONTROLLER_BUTTON_LEFTSTICK)) {
        if ((now - _lastDribblerTime) >= Dribble_Step_Time) {
            _controls.dribblerPower = max(_controls.dribblerPower - 0.1, 0.0);
            _lastDribblerTime = now;
        }
    } else if (SDL_GameControllerGetButton(_controller,
                                           SDL_CONTROLLER_BUTTON_RIGHTSTICK)) {
        if ((now - _lastDribblerTime) >= Dribble_Step_Time) {
            _controls.dribblerPower = min(_controls.dribblerPower + 0.1, 1.0);
            _lastDribblerTime = now;
        }
    } else if (SDL_GameControllerGetButton(_controller,
                                           SDL_CONTROLLER_BUTTON_Y)) {
        /*
         *  DRIBBLER ON/OFF
         */
        if ((now - _lastDribblerTime) >= Dribble_Step_Time) {
            _controls.dribble = !_controls.dribble;
            _lastDribblerTime = now;
        }
    } else {
        // Let dribbler speed change immediately
        _lastDribblerTime = now - Dribble_Step_Time;
    }

    /*
     *  KICKER POWER
     */
    if (SDL_GameControllerGetButton(_controller,
                                    SDL_CONTROLLER_BUTTON_LEFTSHOULDER)) {
        if ((now - _lastKickerTime) >= Kicker_Step_Time) {
            _controls.kickPower = max(_controls.kickPower - 0.1, 0.0);
            _lastKickerTime = now;
        }
    } else if (SDL_GameControllerGetButton(
                   _controller, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)) {
        if ((now - _lastKickerTime) >= Kicker_Step_Time) {
            _controls.kickPower = min(_controls.kickPower + 0.1, 1.0);
            _lastKickerTime = now;
        }
    } else {
        _lastKickerTime = now - Kicker_Step_Time;
    }

    /*
     *  KICK TRUE/FALSE
     */
    _controls.kick =
        SDL_GameControllerGetButton(_controller, SDL_CONTROLLER_BUTTON_A);

    /*
     *  CHIP TRUE/FALSE
     */
    _controls.chip =
        SDL_GameControllerGetButton(_controller, SDL_CONTROLLER_BUTTON_X);

    /*
     *  VELOCITY ROTATION
     */
    // Logitech F310 Controller
    _controls.rotation =
        -1 * SDL_GameControllerGetAxis(_controller, SDL_CONTROLLER_AXIS_LEFTX) /
        AXIS_MAX;

    /*
     *  VELOCITY TRANSLATION
     */
    auto rightX =
        SDL_GameControllerGetAxis(_controller, SDL_CONTROLLER_AXIS_RIGHTX) /
        AXIS_MAX;
    auto rightY =
        -SDL_GameControllerGetAxis(_controller, SDL_CONTROLLER_AXIS_RIGHTY) /
        AXIS_MAX;

    Geometry2d::Point input(rightX, rightY);

    // Align along an axis using the DPAD as modifier buttons
    if (SDL_GameControllerGetButton(_controller,
                                    SDL_CONTROLLER_BUTTON_DPAD_DOWN)) {
        input.y() = -fabs(rightY);
        input.x() = 0;
    } else if (SDL_GameControllerGetButton(_controller,
                                           SDL_CONTROLLER_BUTTON_DPAD_UP)) {
        input.y() = fabs(rightY);
        input.x() = 0;
    } else if (SDL_GameControllerGetButton(_controller,
                                           SDL_CONTROLLER_BUTTON_DPAD_LEFT)) {
        input.y() = 0;
        input.x() = -fabs(rightX);
    } else if (SDL_GameControllerGetButton(_controller,
                                           SDL_CONTROLLER_BUTTON_DPAD_RIGHT)) {
        input.y() = 0;
        input.x() = fabs(rightX);
    }

    // Floating point precision error rounding
    if (_controls.kickPower < 1e-1) _controls.kickPower = 0;
    if (_controls.dribblerPower < 1e-1) _controls.dribblerPower = 0;
    if (fabs(_controls.rotation) < 5e-2) _controls.rotation = 0;
    if (fabs(input.y()) < 5e-2) input.y() = 0;
    if (fabs(input.x()) < 5e-2) input.x() = 0;

    _controls.translation = Geometry2d::Point(input.x(), input.y());
}

JoystickControlValues GamepadController::getJoystickControlValues() {
    QMutexLocker(&mutex());
    return _controls;
}

void GamepadController::reset() {
    QMutexLocker(&mutex());
    _controls.dribble = false;
}

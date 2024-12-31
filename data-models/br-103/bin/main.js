// noinspection DuplicatedCode

import Led from "../lib/led.js";
import {
    warpFlash,
    bussardGlow,
    startImpulseGlow,
    positionLightBlink,
    warpCharge,
    deflectorRotate
} from "../lib/animation.js";

const startupAnimation = async () => {
}

// noinspection JSUnusedGlobalSymbols
const startIdleAnimation = () => {
};

// noinspection JSUnusedGlobalSymbols
const main = () => {
    setIdleAnimationStartHandler(startIdleAnimation);
    delay(0).then(startupAnimation);
};

export {main};

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
    Led.animate('Quarters', 0, '=[0,4000]', 500, 'primary');
    Led.animate('Deflector', 0, '3000/n', '[1000,2000]', 'primary', 3, 'blend', 'easeInOutSine');
    await Led.delayUntilAnimationDone('Deflector');
    Led.animate('Position Lights', 0, 0, 400, 'primary');
    delay(0).then(async () => {
        Led.animate('Impulse', 0, 0, 1000, 'primary(1.3)', 1, 'blend', 'easeInOutSine');
        await Led.delayUntilAnimationDone('Impulse');
        Led.animate('Impulse', 0, 0, 400, 'primary', 1, 'blend', 'easeInOutSine');
    });
    delay(1000).then(() => {
        Led.animate('Front LEDs', 0, 0, 1000, 'primary');
    });
    Led.animate('Bussard', 0, '500/n', 400, 'primary');
    await Led.delayUntilAnimationDone('Bussard');
    await delay(500);
    Led.animate('Warp', 0, '1000/n', 400, 'primary');
    await Led.delayUntilAnimationDone('Warp');
    await delay(2000 + 2000 * Math.random());
    await warpFlash();
}

// noinspection JSUnusedGlobalSymbols
const startIdleAnimation = () => {
    startImpulseGlow();
    delay(0).then(async () => {
        while (true) {
            await delay(20000 + Math.random() * 10000);
            await bussardGlow();
        }
    });
    delay(0).then(async () => {
        while (true) {
            await delay(30000 + Math.random() * 20000);
            await warpCharge();
        }
    });
    delay(0).then(async () => {
        while (true) {
            await delay(40000 + Math.random() * 20000);
            await deflectorRotate();
        }
    });
    delay(0).then(async () => {
        while (true) {
            await delay(5000 + Math.random() * 10000);
            Led.setPixel('Quarters', Math.floor(Math.random() * 20), Math.random() > 0.5 ? 'primary' : 'primary(0)');
        }
    });
    delay(0).then(async () => {
        while (true) {
            await delay(30000 + Math.random() * 20000);
            await positionLightBlink();
        }
    });
};

// noinspection JSUnusedGlobalSymbols
const main = () => {
    setIdleAnimationStartHandler(startIdleAnimation);
    delay(0).then(startupAnimation);
};

export {main};
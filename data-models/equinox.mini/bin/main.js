import Led from "../lib/led.js";
import {warpFlash, bussardGlow, warpCharge, deflectorRotate} from "../lib/animation.js";

// noinspection JSUnusedGlobalSymbols
export default function() {
    onIdleAnimationStart(() => {
        delay(0).then(idleAnimation);
    });
}

const startupAnimation = async () => {
    Led.animate('Deflector', 0, 0, '[1000,2000]', 'primary');
    await Led.delayUntilAnimationDone('Deflector');
    Led.animate('Bussard', 0, '400/n', 400, 'primary');
    await Led.delayUntilAnimationDone('Bussard');
    await delay(500);
    Led.animate('Warp', 0, '1000/n', 400, 'primary');
    await Led.delayUntilAnimationDone('Warp');
    await warpFlash();
}

// noinspection JSUnusedGlobalSymbols
const startIdleAnimation = () => {
    delay(0).then(async () => {
        while (true) {
            await delay(20000 + Math.random() * 10000);
            await bussardGlow();
        }
    });
    delay(0).then(async () => {
        while (true) {
            await delay(30000 + Math.random() * 10000);
            await warpCharge();
        }
    });
    delay(0).then(async () => {
        while (true) {
            await delay(40000 + Math.random() * 20000);
            await deflectorRotate();
        }
    });
};

// noinspection JSUnusedGlobalSymbols
const main = () => {
    setIdleAnimationStartHandler(startIdleAnimation);
    delay(0).then(startupAnimation);
};

export {main};

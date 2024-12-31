import Led from "../lib/led.js";
import {randomPointOnSphere} from "../lib/animation.js";

const startupAnimation = async () => {
    Led.animate('All', 0, '=[0,4000]', '[200,400]', 'blue');
    await Led.delayUntilAnimationDone('All');
    Led.animate3D('Cube', 'local', randomPointOnSphere(8), 0, 80, 400, 0, 'primary');
    await Led.delayUntilAnimationDone('Cube');
}

const energyFlash = async () => {
    Led.animate('All', 0, '=[0,600]', '[200,400]', 'primary', 2, 'add', 'easeInOutSine');
    await Led.delayUntilAnimationDone('All');
    await delay(200);
    Led.animate3D('Cube', 'local', randomPointOnSphere(8), 0, 40, 200, 0, 'red');
    await Led.delayUntilAnimationDone('All');
    await delay(4000 + Math.random() * 1000);
    Led.animate('All', 0, '=[0,4000]', '[200,400]', 'primary');
    await Led.delayUntilAnimationDone('All');
}

const energyLoss = async () => {
    const startPos = randomPointOnSphere(8);
    Led.animate3D('All', 'local', startPos, 0, 50, 400, 0, 'primary(0.3)', 2, 'add', 'easeInOutSine');
    await delay(400);
    Led.animate3D('All', 'local', startPos, 0, 50, 400, 0, 'primary(0)');
    await Led.delayUntilAnimationDone('All');
    await delay(2000);
    await startupAnimation();
}

// noinspection JSUnusedGlobalSymbols
const startIdleAnimation = () => {
    delay(0).then(async () => {
        while (true) {
            await delay(40000 + Math.random() * 20000);
            if (Math.random() > 0.3) {
                await energyFlash();
            } else {
                await energyLoss();
            }
        }
    });
};

// noinspection JSUnusedGlobalSymbols
const main = () => {
    setIdleAnimationStartHandler(startIdleAnimation);
    delay(0).then(startupAnimation);
};

export {main, energyLoss, energyFlash};

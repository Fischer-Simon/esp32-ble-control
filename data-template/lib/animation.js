import Led from "../lib/led.js"

const randomPointOnSphere = (radius = 1) => {
    // Generate theta in [0, 2π]
    const theta = 2 * Math.PI * Math.random();

    // Generate phi in [0, π] with correct distribution
    const phi = Math.acos(2 * Math.random() - 1);

    // Convert spherical coordinates to Cartesian coordinates
    const x = radius * Math.sin(phi) * Math.cos(theta);
    const y = radius * Math.sin(phi) * Math.sin(theta);
    const z = radius * Math.cos(phi);

    return [x, y, z];
};

const warpCharge = async () => {
    Led.animate('Warp', 0, 20, 300, 'primary(0)');
    await delay(Led.getAnimationTimeLeft('Warp') + Math.random() * 1000);
    Led.animate('Warp', 0, 20, 400, 'primary(0.2)');
    await delay(Led.getAnimationTimeLeft('Warp'));
    Led.animate('Warp', 0, 60, 6000, 'primary(1)', -14, 'blend', 'easeInOutSine');
    await delay(Led.getAnimationTimeLeft('Warp'));
    Led.animate('Warp', 0, 100, 1000, 'primary(1)');
    await delay(Led.getAnimationTimeLeft('Warp'));
};

const warpFlash = async () => {
    Led.animate('Warp', 0, '1000/n', 400, 'primary(1.5)');
    await Led.delayUntilAnimationDone('Warp');
    Led.animate('Warp', 0, 0, 100, 'rgb(0,1.5,2)');
    Led.animate('Warp', 100, 0, 800, 'primary(1)', 1, 'blend', 'easeOutSine');
    await Led.delayUntilAnimationDone('Warp');
};

const startImpulseGlow = () => {
    delay(0).then(async () => {
        while (true) {
            Led.animate('Impulse', 0, 0, 3000, 'primary(0.2)', 2, 'add', 'easeInOutSine');
            await delay(1000);
        }
    });
};

const positionLightBlink = async () => {
    Led.animate('Position Lights', 0, 0, 200, 'primary(0.4)', 2, 'add', 'easeInOutQuart');
    await delay(1000);
    Led.animate('Position Lights', 0, 0, 200, 'primary(0.4)', 2, 'add', 'easeInOutQuart');
    await delay(1000);
}

const bussardGlow = async () => {
    Led.animate('Bussard', 0, '500/n', 200, 'primary(0)');
    await Led.delayUntilAnimationDone('Bussard');
    await delay(400);
    Led.animate('Bussard', 0, '2000/n', 800, 'primary(1.2)', 2, 'blend', 'easeInOutSine');
    await delay(1000);
    Led.animate('Bussard', 0, '2000/n', 800, 'primary(1.2)', 2, 'blend', 'easeInOutSine');
    await Led.delayUntilAnimationDone('Bussard');
    Led.animate('Bussard', 0, '1000/n', 1000, 'primary');
    await Led.delayUntilAnimationDone('Bussard');
}

const deflectorRotate = async () => {
    Led.animate('Deflector', 0, '=[0,800]', 300, 'primary(0.5)');
    await Led.delayUntilAnimationDone('Deflector');
    Led.animate('Deflector', 0, '1000/n', 2000, 'primary', 4, 'add', 'easeInOutSine');
    await delay(2000);
    Led.animate('Deflector', 0, '=[0,800]', 300, 'primary');
    await Led.delayUntilAnimationDone('Deflector');
};

const lightWave = (distance) => {
    Led.animate3D('All', 'local', randomPointOnSphere(distance), 0, 40, '[400,800]', 0, 'primary', 2, 'add', 'easeInOutSine');
}

export {randomPointOnSphere, warpCharge, warpFlash, startImpulseGlow, positionLightBlink, bussardGlow, deflectorRotate, lightWave};

import Led from "../lib/led.js";

const flash = async() => {
    const hue = Math.random();
    Led.animate('Mirror', 0, 200, 300, 'hsl(' + hue + ',1,0.5)');
    await delay(800);
    Led.animate('Mirror', 0, 200, 300, "hsl(" + hue + ",1,0.1)");
    await delay(Led.getAnimationTimeLeft('Mirror'));
}

const policeBlink = async () => {
    for (let i = 0; i < 25; i++) {
        Led.animate3D('Mirror', 'local', [10,0,2], 0, 20, 200, 4, 'blue', 16, 'blend', 'easeInOutExpo');
        Led.animate3D('Mirror', 'local', [5,0,2], 200, 20, 200, 4, 'red', 16, 'blend', 'easeInOutExpo');
        await delay(400);
    }
}

const startIdleAnimation = () => {
    delay(0).then(async () => {
        while (true) {
            Led.animate('mirror', 0, 200, 300, 'primary(1)');
            await delay(2000 + 1000 * Math.random());
            await flash();
            await delay(1600);
        }
    });
};


// noinspection JSUnusedGlobalSymbols
const main = () => {
    setIdleAnimationStartHandler(startIdleAnimation);
};

export {main};

const flash = async() => {
    const hue = Math.random();
    run("led", "animate", "Mirror", 0, 200, 300, "hsl(" + hue + ",1, 0.5)");
    await delay(800)
    run("led", "animate", "Mirror", 0, 200, 300, "hsl(" + hue + ",1,0.1)");
}

const policeBlink = async () => {
    for (let i = 0; i < 25; i++) {
        run('led', 'animate-3d', 'Mirror', '10,0,2', 0, 20, 200, 4, 'blue', 16, 'blendManual', 'easeInOutExpo');
        run('led', 'animate-3d', 'Mirror', '5,0,2', 200, 20, 200, 4, 'red', 16, 'blendManual', 'easeInOutExpo');
        await delay(400);
    }
}

onIdleAnimationStart(() => {
    delay(0).then(async () => {
        while (true) {
            run("led", "animate", "Mirror", 0, 200, 300, "primary(1)");
            await delay(2000 + 1000 * Math.random());
            await flash();
            await delay(1600);
        }
    });
});

const Led = {
    setPixel: (ledString, ledIndex, color) => {
        run('led', 'set-pixel', ledString, ledIndex, color);
    },
    animate: (ledString, delay, pixelDelay, pixelDuration, color, halfCycles, blending, easing) => {
        if (halfCycles !== undefined) {
            run(
                'led', 'animate',
                ledString, delay, pixelDelay, pixelDuration, color, halfCycles, blending, easing
            );
        } else {
            run(
                'led', 'animate',
                ledString, delay, pixelDelay, pixelDuration, color
            );
        }
    },
    animate3D: (ledString, coordType, startPos, delay, pixelDelay, pixelDuration, range, color, halfCycles, blending, easing) => {
        if (typeof startPos === "object") {
            startPos = `[${startPos[0]},${startPos[1]},${startPos[2]}]`;
        }
        if (halfCycles !== undefined) {
            run(
                'led', 'animate-3d',
                ledString, coordType, startPos, delay, pixelDelay, pixelDuration, range, color, halfCycles, blending, easing
            );
        } else {
            run(
                'led', 'animate-3d',
                ledString, coordType, startPos, delay, pixelDelay, pixelDuration, range, color
            );
        }
    },
    getAnimationTimeLeft: (ledString) => {
        return parseInt(run('return', 'led', 'animation-time-left', ledString));
    },
    delayUntilAnimationDone: (ledString) => {
        return delay(Led.getAnimationTimeLeft(ledString));
    }
};

export default Led;

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

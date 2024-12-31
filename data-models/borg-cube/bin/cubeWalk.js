/**
 * Generates a random point on the surface of a cube.
 * @param {number} min - The minimum coordinate value.
 * @param {number} max - The maximum coordinate value.
 * @returns {{x: number, y: number, z: number}} - A point on the cube's surface.
 */
function randomPointOnCubeSurface(min, max) {
    // Helper function to generate a random coordinate between min and max
    const randomCoord = () => min + Math.random() * (max - min);

    // Choose one of the six faces randomly
    const face = Math.floor(Math.random() * 6);

    let x, y, z;

    switch (face) {
        case 0: // +X face
            x = max;
            y = randomCoord();
            z = randomCoord();
            break;
        case 1: // -X face
            x = min;
            y = randomCoord();
            z = randomCoord();
            break;
        case 2: // +Y face
            y = max;
            x = randomCoord();
            z = randomCoord();
            break;
        case 3: // -Y face
            y = min;
            x = randomCoord();
            z = randomCoord();
            break;
        case 4: // +Z face
            z = max;
            x = randomCoord();
            y = randomCoord();
            break;
        case 5: // -Z face
            z = min;
            x = randomCoord();
            y = randomCoord();
            break;
    }

    x = Math.floor(x);
    y = Math.floor(y);
    z = Math.floor(y);

    return { x, y, z };
}

/**
 * Determines the current face based on the position.
 * @param {number} x
 * @param {number} y
 * @param {number} z
 * @param {number} min
 * @param {number} max
 * @returns {string} - The current face identifier.
 */
function getCurrentFace(x, y, z, min, max) {
    if (x === max) return '+X';
    if (x === min) return '-X';
    if (y === max) return '+Y';
    if (y === min) return '-Y';
    if (z === max) return '+Z';
    if (z === min) return '-Z';
    return null; // Should not happen if the position is on the surface
}

/**
 * Returns the allowed directions based on the current face.
 * Directions are represented as unit vectors.
 * @param {string} face
 * @returns {Array<{dx: number, dy: number, dz: number}>} - Allowed directions.
 */
function getAllowedDirections(face) {
    switch (face) {
        case '+X':
        case '-X':
            return [
                { dx: 0, dy: 1, dz: 0 },  // +Y
                { dx: 0, dy: -1, dz: 0 }, // -Y
                { dx: 0, dy: 0, dz: 1 },  // +Z
                { dx: 0, dy: 0, dz: -1 }  // -Z
            ];
        case '+Y':
        case '-Y':
            return [
                { dx: 1, dy: 0, dz: 0 },  // +X
                { dx: -1, dy: 0, dz: 0 }, // -X
                { dx: 0, dy: 0, dz: 1 },  // +Z
                { dx: 0, dy: 0, dz: -1 }  // -Z
            ];
        case '+Z':
        case '-Z':
            return [
                { dx: 1, dy: 0, dz: 0 },  // +X
                { dx: -1, dy: 0, dz: 0 }, // -X
                { dx: 0, dy: 1, dz: 0 },  // +Y
                { dx: 0, dy: -1, dz: 0 }  // -Y
            ];
        default:
            return [];
    }
}

/**
 * Generates a random walk on the surface of a cube.
 * @param {number} min - The minimum coordinate value of the cube.
 * @param {number} max - The maximum coordinate value of the cube.
 * @param {number} steps - The number of steps in the walk.
 * @param {number} changeProb - Probability (0 to 1) to change direction at each step.
 * @returns {Array<{x: number, y: number, z: number}>} - The path of the walk.
 */
export default function(min, max, steps, changeProb) {
    // Initialize path array
    const path = [];

    // Generate random starting position
    let currentPos = randomPointOnCubeSurface(min, max);
    path.push({ ...currentPos });

    // Determine the current face
    let currentFace = getCurrentFace(currentPos.x, currentPos.y, currentPos.z, min, max);
    if (!currentFace) {
        throw new Error(`Invalid starting position ${currentPos.x},${currentPos.y},${currentPos.z}: Not on the cube surface.`);
    }

    // Get allowed directions for the current face
    let allowedDirections = getAllowedDirections(currentFace);

    // Randomly select an initial direction
    let currentDir = allowedDirections[Math.floor(Math.random() * allowedDirections.length)];

    for (let i = 0; i < steps; i++) {
        // Decide whether to change direction
        if (Math.random() < changeProb) {
            // Select a new direction different from the current one
            const possibleDirs = allowedDirections.filter(dir =>
                !(dir.dx === -currentDir.dx && dir.dy === -currentDir.dy && dir.dz === -currentDir.dz)
            );
            currentDir = possibleDirs[Math.floor(Math.random() * possibleDirs.length)];
        }

        // Move one unit in the current direction
        let newPos = {
            x: currentPos.x + currentDir.dx,
            y: currentPos.y + currentDir.dy,
            z: currentPos.z + currentDir.dz
        };

        // Check if the new position is still on the same face
        let newFace = getCurrentFace(newPos.x, newPos.y, newPos.z, min, max);

        if (newFace === currentFace) {
            // Still on the same face, update position
            currentPos = newPos;
        } else {
            // Transition to a new face
            // Determine which face we moved to
            if (!newFace) {
                // If the move is out of bounds, revert to previous position
                // Alternatively, handle wrapping or other boundary conditions
                console.warn('Attempted to move out of cube bounds. Staying on current position.');
                newPos = { ...currentPos };
            } else {
                // Update to the new face
                currentFace = newFace;
                allowedDirections = getAllowedDirections(currentFace);

                // Adjust direction to be compatible with the new face
                // For simplicity, keep the same direction if possible, else choose a new one
                const directionPossible = allowedDirections.some(dir =>
                    dir.dx === currentDir.dx && dir.dy === currentDir.dy && dir.dz === currentDir.dz
                );

                if (!directionPossible) {
                    currentDir = allowedDirections[Math.floor(Math.random() * allowedDirections.length)];
                }

                currentPos = newPos;
            }
        }

        // Add the new position to the path
        path.push({ ...currentPos });
    }

    return path;
}

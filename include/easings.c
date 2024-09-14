inline float easeInOutCubic(float x) {
    if (x < 0.5) {
        return 4 * x * x * x;
    } else {
        return 1 - powf(-2 * x + 2, 3) / 2;
    }
}

inline float easeInSine(float x) {
    return 1 - cos((x * PI32) / 2);
}

inline float easeOutBounce(float x) {
	const float n1 = 7.5625;
	const float d1 = 2.75;

	if (x < 1 / d1) {
		return n1 * x * x;
	} else if (x < 2 / d1) {
		x -= 1.5 / d1;
		return n1 * x * x + 0.75;
	} else if (x < 2.5 / d1) {
		x -= 2.25 / d1;
    	return n1 * x * x + 0.9375;
	} else {
		x -= 2.625 / d1;
		return n1 * x * x + 0.984375;
	}
}
#ifndef UTIL_H
#define UTIL_H 1

int randomBetween(int lo, int hi) {
	return lo + rand() / (RAND_MAX / (hi - lo) + 1);
}

#endif /* UTIL_H */

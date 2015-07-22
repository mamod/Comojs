/**
* moves the cursor to the x and y coordinate on the given stream
*/
function cursorTo(stream, x, y) {
	if (util.isNullOrUndefined(stream))
		return;
	
	if (!util.isNumber(x) && !util.isNumber(y))
		return;
	
	if (!util.isNumber(x))
		throw new Error("Can't set cursor row without also setting it's column");

	if (!util.isNumber(y)) {
		stream.write('\x1b[' + (x + 1) + 'G');
	} else {
		stream.write('\x1b[' + (y + 1) + ';' + (x + 1) + 'H');
	}
}
exports.cursorTo = cursorTo;

/**
* moves the cursor relative to its current location
*/
function moveCursor(stream, dx, dy) {
	if (util.isNullOrUndefined(stream))
		return;

	if (dx < 0) {
		stream.write('\x1b[' + (-dx) + 'D');
	} else if (dx > 0) {
		stream.write('\x1b[' + dx + 'C');
	}

	if (dy < 0) {
		stream.write('\x1b[' + (-dy) + 'A');
	} else if (dy > 0) {
		stream.write('\x1b[' + dy + 'B');
	}
}
exports.moveCursor = moveCursor;

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// Copyright 2022 John Maloney, Bernat Romagosa, and Jens Mönig

// boardie.c - Boardie - A Simulated MicroBlocks Board for Web Browsers
// John Maloney and Bernat Romagosa, October 2022

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include <emscripten.h>

#include "mem.h"
#include "interp.h"
#include "persist.h"

// Timing Functions

static int startSecs = 0;

static void initTimers() {
	struct timeval now;
	gettimeofday(&now, NULL);
	startSecs = now.tv_sec;
}

uint32 microsecs() {
	struct timeval now;
	gettimeofday(&now, NULL);

	return (1000000 * (now.tv_sec - startSecs)) + now.tv_usec;
}

uint32 millisecs() {
	struct timeval now;
	gettimeofday(&now, NULL);

	return (1000 * (now.tv_sec - startSecs)) + (now.tv_usec / 1000);
}

// Communication/System Functions

void initMessageService() {
	EM_ASM_({
		window.recvBuffer = [];
		window.addEventListener('message', function (event) {
			window.recvBuffer.push(...event.data);
		}, false);
	});
}

int nextByte() {
	return EM_ASM_INT({
		// Returns first byte in the buffer, and removes it from the buffer
		return window.recvBuffer.splice(0, 1)[0];
	});
}

int canReadByte() {
	return EM_ASM_INT({
		if (!window.recvBuffer) { window.recvBuffer = []; }
		return window.recvBuffer.length > 0;
	});
}

int recvBytes(uint8 *buf, int count) {
	int total = 0;
	while (canReadByte() && total <= count) {
		buf[total] = nextByte();
		total++;
	}
	return total;
}

int sendBytes(uint8 *buf, int start, int end) {
	EM_ASM_({
		var bytes = new Uint8Array($2 - $1);
		for (var i = $1; i < $2; i++) {
			bytes[i - $1] = getValue($0 + i, 'i8');
		}
		window.parent.postMessage(bytes);
	}, buf, start, end);
	return end - start;
}

// Keyboard support
void initKeyboardHandler() {
	EM_ASM_({
		window.keys = new Map();

		window.buttons = [];
		window.buttons[37] = // left cursor
			window.parent.document.querySelector('[data-button="a"]');
		window.buttons[65] = window.buttons[37]; // "a" key
		window.buttons[39] =
			window.parent.document.querySelector('[data-button="b"]');
		window.buttons[66] = window.buttons[39]; // "b" key

		window.addEventListener('keydown', function (event) {
			if (window.buttons[event.keyCode]) {
				window.buttons[event.keyCode].classList.add('--is-active');
			}
			window.keys.set(event.keyCode, true);
		}, false);
		window.addEventListener('keyup', function (event) {
			if (window.buttons[event.keyCode]) {
				window.buttons[event.keyCode].classList.remove('--is-active');
			}
			window.keys.set(event.keyCode, false);
		}, false);
	});
}

// Sound support
void initSound() {
	EM_ASM_({
		var context = new AudioContext();
		window.gainNode = context.createGain();
		window.gainNode.gain.value = 0.1;
		window.oscillator = context.createOscillator();
		window.oscillator.type = 'square';
		window.oscillator.start();
		window.gainNode.connect(context.destination);
	});
};

// System Functions

const char * boardType() {
	return "Boardie";
}

// Grab ublockscode as a base64 URL
void EMSCRIPTEN_KEEPALIVE getScripts() {
	compactCodeStore();
	EM_ASM_({
		console.log(
			encodeURIComponent(
				btoa(
					String.fromCharCode.apply(
						null,
						new Uint8Array(HEAP8.subarray($0, $0 + $1))
					)
				)
			)
		);
	}, ramStart(), ramSize());
}

void readScriptsFromURL() {
	EM_ASM_({
		var b64 = (new URLSearchParams(window.location.search)).get('code');
		if (b64) {
			var bytes = Int8Array.from(atob(b64), (c) => c.charCodeAt(0));
			for (var i = 0; i < bytes.length; i++) {
				setValue($0, bytes[i], 'i8');
				$0++;
			}
		}
	}, ramStart());
	restoreScripts();
	startAll();
}

// Stubs for functions not used by Boardie

void addSerialPrims() {}
void delay(int msecs) {}
void processFileMessage(int msgType, int dataSize, char *data) {}

// Stubs for code file (persistence) not yet used by Boardie

void initCodeFile(uint8 *flash, int flashByteCount) {}
void writeCodeFile(uint8 *code, int byteCount) { }
void writeCodeFileWord(int word) { }
void clearCodeFile(int ignore) { }

// Main loop

int main(int argc, char *argv[]) {
	printf("Starting Boardie\n");

	initMessageService();
	initKeyboardHandler();
	initSound();

	initTimers();
	memInit();
	primsInit();
	restoreScripts();
	startAll();
	readScriptsFromURL();

	printf("Starting interpreter\n");
	emscripten_set_main_loop(interpretStep, 60, true); // callback, fps, loopFlag
}
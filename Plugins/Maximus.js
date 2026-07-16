export function Name() { return "Maximus RGB Controller"; }
export function VendorId() { return 0x7331; }
export function ProductId() { return 0x000B; }
export function Publisher() { return "Maximus"; }
export function Type() { return "rawusb"; } 
export function DeviceType() { return "lightingcontroller"; }
export function SubdeviceController() { return true; }
export function ImageUrl() { return "https://raw.githubusercontent.com/MrMa1ik/Maximus-RGB-Controller/refs/heads/main/Assets/Maximus.png"; }

export function Validate(endpoint) { 
	// Targets Interface 0 now that CDC Serial is programmatically disabled
	return endpoint.interface === 0; 
}


/* global
LightingMode:readonly
forcedColor:readonly
hardwareEffect:readonly
hardwareColor:readonly
hardwareColor2:readonly
hardwareBrightness:readonly
hardwareSpeed:readonly
*/

export function ControllableParameters() {
	return [
		{
			property: "LightingMode",
			group: "lighting",
			label: "Lighting Mode",
			description: "Canvas uses SignalRGB effects. Forced uses one color. Hardware Effect saves an effect to the Pico.",
			type: "combobox",
			values: ["Canvas", "Forced", "Hardware Effect"],
			default: "Canvas",
		},
		{
			property: "forcedColor",
			group: "lighting",
			label: "Forced Color",
			type: "color",
			default: "#009bde",
		},
		{
			property: "hardwareEffect",
			group: "lighting",
			label: "Hardware Effect",
			type: "combobox",
			values: [
				"Static",
				"Breathing",
				"Flashing",
				"Double flashing",
				"Color pulse",
				"Color shift",
				"Color wave",
				"Marquee",
				"Rainbow wave",
				"Visor",
				"Rainbow flashing",
				"Color ring double flashing",
				"Stack",
				"Fire",
				"Lighting",
				"Meteor",
				"Color ring",
				"Planetary",
				"Double meteor",
				"Energy",
				"Blink",
				"Clock",
				"Sequential Glow"
			],
			default: "Rainbow wave",
		},
		{
			property: "hardwareColor",
			group: "lighting",
			label: "Hardware Color",
			type: "color",
			default: "#ff0000",
		},
		{
			property: "hardwareColor2",
			group: "lighting",
			label: "Hardware Color 2 (Secondary)",
			type: "color",
			default: "#ff00ff",
		},
		{
			property: "hardwareBrightness",
			group: "lighting",
			label: "Hardware Brightness",
			type: "number",
			min: 0,
			max: 255,
			default: 120,
		},
		{
			property: "hardwareSpeed",
			group: "lighting",
			label: "Hardware Speed",
			type: "number",
			min: 0,
			max: 255,
			default: 128,
		},
	];
}

const ChannelCount = 4;
const ChannelLed = 250;

// 9 bytes header (1 byte CMD + 4 channels * 2 bytes length) + 3000 bytes payload
const SerialReportSize = 3009; 

const DeviceMaxLedLimit = ChannelCount * ChannelLed;

const CMD_SIGNALRGB_SHOW = 0x03;

let lastHardwareState = "";
let currentFpsTarget = 60; // Track the active FPS target

const ChannelArray = [
	["Channel 01", ChannelLed],
	["Channel 02", ChannelLed],
	["Channel 03", ChannelLed],
	["Channel 04", ChannelLed],
];

const EffectIds = {
	"Static": 1,
	"Breathing": 2,
	"Flashing": 3,
	"Double flashing": 4,
	"Color pulse": 5,
	"Color shift": 6,
	"Color wave": 7,
	"Marquee": 8,
	"Rainbow wave": 9,
	"Visor": 10,
	"Rainbow flashing": 11,
	"Color ring double flashing": 12,
	"Stack": 13,
	"Fire": 14,
	"Lighting": 15,
	"Meteor": 16,
	"Color ring": 17,
	"Planetary": 18,
	"Double meteor": 19,
	"Energy": 20,
	"Blink": 21,
	"Clock": 22,
	"Sequential Glow": 23,
};

const ColorEffects = new Set([
	"Static",
	"Breathing",
	"Flashing",
	"Double flashing",
	"Color pulse",
	"Marquee",
	"Visor",
	"Stack",
	"Meteor",
	"Planetary",
	"Double meteor",
	"Blink",
	"Clock",
	"Sequential Glow"
]);

export function Initialize() {
	device.SetLedLimit(DeviceMaxLedLimit);

	for (let i = 0; i < ChannelArray.length; i++) {
		device.addChannel(ChannelArray[i][0], ChannelArray[i][1]);
	}

	// Set constant frame rate target of 60 FPS
	currentFpsTarget = 60;
	device.setFrameRateTarget(currentFpsTarget);
}

export function Render() {
	if (LightingMode === "Hardware Effect") {
		ApplyHardwareEffect();
		return;
	}

	lastHardwareState = "";

	const packet = new Array(SerialReportSize).fill(0);
	packet[0] = CMD_SIGNALRGB_SHOW;

	let bytes_offset = 9;
	let totalActiveLeds = 0; // Accumulator for dynamic FPS calculation

	for (let channel = 0; channel < ChannelCount; channel++) {
		const channelName = ChannelArray[channel][0];
		const componentChannel = device.channel(channelName);
		
		let channelLedCount = 0;
		let rgbData = null;

		if (componentChannel !== undefined && componentChannel !== null) {
			if (LightingMode === "Forced") {
				rgbData = device.createColorArray(forcedColor, componentChannel.LedCount(), "Inline", "RGB");
			} else {
				rgbData = componentChannel.getColors("Inline", "RGB");
			}
			totalActiveLeds += componentChannel.LedCount();
		}

		if (rgbData && rgbData.length > 0) {
			channelLedCount = Math.min(ChannelLed, rgbData.length / 3);
			
			// Bound check matching the 3000 bytes maximum payload limit
			if (bytes_offset + channelLedCount * 3 > 3009) {
				channelLedCount = Math.floor((3009 - bytes_offset) / 3);
			}
			
			for (let j = 0; j < channelLedCount * 3; j++) {
				packet[bytes_offset + j] = rgbData[j];
			}
			bytes_offset += channelLedCount * 3;
		}

		const headerOffset = 1 + (channel * 2);
		packet[headerOffset] = channelLedCount & 0xFF;
		packet[headerOffset + 1] = (channelLedCount >> 8) & 0xFF;
	}

	// Writes to virtualized Endpoint 0x01 on Interface 0
	device.bulk_transfer(0x01, packet, SerialReportSize, 100);
}

function ApplyHardwareEffect() {
	const effectId = EffectIds[hardwareEffect] ?? EffectIds["Rainbow wave"];
	const brightness = Clamp(hardwareBrightness, 0, 255);
	const speed = Clamp(hardwareSpeed, 0, 255);
	const color = HexToRgb(hardwareColor);
	const color2 = HexToRgb(hardwareColor2);
	const state = effectId + ":" + brightness + ":" + speed + ":" + color.join(",") + ":" + color2.join(",");

	if (state === lastHardwareState) return;

	lastHardwareState = state;

	for (let channel = 0; channel < ChannelCount; channel++) {
		const channelName = ChannelArray[channel][0];
		const componentChannel = device.channel(channelName);
		
		// Query the layout to find the configured count of added components on this channel
		let ledCount = ChannelLed; // Default fallback to 250
		if (componentChannel !== undefined && componentChannel !== null) {
			ledCount = componentChannel.LedCount();
		}

		SendTextCommand("COUNT " + channel + " " + ledCount);
		SendTextCommand("BRIGHTNESS " + channel + " " + brightness);
		SendTextCommand("SPEED " + channel + " " + speed);
		
		if (ColorEffects.has(hardwareEffect)) {
			SendTextCommand("COLOR " + channel + " " + color[0] + " " + color[1] + " " + color[2]);
		}
		
		// Send custom hardware parameters
		SendTextCommand("COLOR2 " + channel + " " + color2[0] + " " + color2[1] + " " + color2[2]);
		
		SendTextCommand("EFFECT " + channel + " " + effectId);
	}

	SendTextCommand("SAVE");
}

function SendTextCommand(command) {
	const packet = [];
	for (let i = 0; i < command.length; i++) {
		packet.push(command.charCodeAt(i));
	}
	packet.push(10); // \n
	// Writes to virtualized Endpoint 0x01 on Interface 0
	device.bulk_transfer(0x01, packet, packet.length, 100);
	device.pause(5);
}

// Helper functions below are untouched
function Clamp(value, min, max) {
	value = Number(value);
	if (Number.isNaN(value)) return min;
	if (value < min) return min;
	if (value > max) return max;
	return value;
}

function HexToRgb(hex) {
	const match = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex || "#ff0000");
	if (!match) return [255, 0, 0];
	return [
		parseInt(match[1], 16),
		parseInt(match[2], 16),
		parseInt(match[3], 16),
	];
}
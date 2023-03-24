class AudioProcessor extends AudioWorkletProcessor {
	buffer = [null, null];
	length = 0;
	offset = 0;

	constructor(options) {
		console.log('constructor');
		super(options);

		this.port.onmessage = this.messages.bind(this);

		let pointers = options.processorOptions['pointers'];
		let buffer = options.processorOptions['buffer'];
		let length = options.processorOptions['length'];

		this.length = length;
		this.buffer[0] = new Float32Array(buffer, pointers[0], length);
		this.buffer[1] = new Float32Array(buffer, pointers[1], length);
	}

	messages(e) { console.log(e); }

	process(inputs, outputs, parameters) {
		const output = outputs[0];
		const channelL = output[0];
		const channelR = output[1];

		for (let i = 0; i < channelL.length; i += 1) {
			channelL[i] = this.buffer[0][i + this.offset];
		}
		for (let i = 0; i < channelR.length; i += 1) {
			channelR[i] = this.buffer[1][i + this.offset];
		}
		this.offset += 128;

		// Request new audio samples if half of the buffer has been consumed
		if (this.offset == 1024) {
			this.port.postMessage(0);
		} else if (this.offset == 2048) {
			this.port.postMessage(1024);
			this.offset = 0;
		}
		return true;
	}
}

registerProcessor('vAmiga_audioprocessor_sharedarraybuffer', AudioProcessor);
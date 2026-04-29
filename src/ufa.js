addToLibrary({
	$ufa_init__postset: 'ufa_init();',
	$ufa_init: () => {
		const handles = new Map();
		let nextHandle = 0;

		function allocHandle(obj) {
			const handle = nextHandle++;
			handles.set(handle, obj);
			return handle;
		}

		function closeHandle(handle) {
			handles.delete(handle);
		}

		function resolveHandle(handle) {
			return handles.get(handle);
		}

		const UFA_OK = 0;
		const UFA_PENDING = 1;
		const UFA_CANCELLED = 2;
		const UFA_ERROR = 3;

		_ufa_web_begin_open_file = (filter, alloc_ctx, filename_ptr, status_ptr) => {
			const input = document.createElement('input');
			input.type = 'file';

			const obj = {
				input,
				alloc_ctx,
				content: null,
				pos: 0,
			};

			function openSuccess(file) {
				obj.file = file;

				const nameLen = lengthBytesUTF8(file.name) + 1;
				const nameBuf = _ufa_web_malloc(alloc_ctx, nameLen);
				stringToUTF8(file.name, nameBuf, nameLen);
				setValue(filename_ptr, nameBuf, '*');

				file.arrayBuffer().then((buf) => {
					obj.content = new Uint8Array(buf);
					setValue(status_ptr, UFA_OK, 'i8');
				}).catch((e) => {
					console.error(e);
					setValue(status_ptr, UFA_ERROR, 'i8');
				})
			}

			function openFailure() {
				setValue(status_ptr, UFA_CANCELLED);
			}

			input.addEventListener("change", (event) => {
				if (input.files.length === 1) {
					openSuccess(input.files[0]);
				} else {
					openFailure();
				}
			}, {once: true});

			input.addEventListener("cancel", (event) => {
				openFailure();
			}, {once: true});

			input.accept = UTF8ToString(filter);
			console.log(input.accept);
			input.click();

			return allocHandle(obj);
		};

		_ufa_web_end_open_file = (handle) => {
			closeHandle(handle);
		};

		_ufa_web_read_open_file = (handle, buf, size_ptr) => {
			try {
				const file = resolveHandle(handle);
				if (!file || !file.content) {
					HEAPU32[size_ptr >> 2] = 0;
					return UFA_ERROR;
				}

				const bytesRequested = HEAPU32[size_ptr >> 2];

				if (file.pos >= file.content.byteLength) {
					HEAPU32[size_ptr >> 2] = 0;
					return UFA_OK;
				}

				const bytesAvailable = file.content.byteLength - file.pos;
				const bytesToRead = Math.min(bytesRequested, bytesAvailable);

				HEAPU8.set(file.content.subarray(file.pos, file.pos + bytesToRead), buf);
				file.pos += bytesToRead;

				HEAPU32[size_ptr >> 2] = bytesToRead;
				return UFA_OK;
			} catch (e) {
				console.error(e);
				return UFA_ERROR;
			}
		};

		_ufa_web_begin_save_file = (filename) => {
			return allocHandle({
				filename: UTF8ToString(filename),
				chunks: [],
			});
		};

		_ufa_web_end_save_file = (handle) => {
			const file = resolveHandle(handle);
			if (!file) { return; }

			const blob = new Blob(file.chunks, { type: 'application/octet-stream' });
			const url = URL.createObjectURL(blob);

			const a = document.createElement('a');
			a.href = url;
			a.download = file.filename;
			a.click();

			URL.revokeObjectURL(url);
			closeHandle(handle);
		};

		_ufa_web_write_save_file = function(handle, buf, size_ptr) {
			try {
				const file = resolveHandle(handle);
				if (!file) { return UFA_ERROR; }

				const bytesToWrite = HEAPU32[size_ptr >> 2];
				const chunk = HEAPU8.slice(buf, buf + bytesToWrite);
				file.chunks.push(chunk);
				return UFA_OK;
			} catch (e) {
				console.error(e);
				return UFA_ERROR;
			}
		};
	},

	ufa_web_begin_open_file: () => {},
	ufa_web_begin_open_file__deps: ['$ufa_init'],

	ufa_web_end_open_file: () => {},
	ufa_web_end_open_file__deps: ['$ufa_init'],

	ufa_web_read_open_file: () => {},
	ufa_web_read_open_file__deps: ['$ufa_init'],

	ufa_web_begin_save_file: () => {},
	ufa_web_begin_save_file__deps: ['$ufa_init'],

	ufa_web_end_save_file: () => {},
	ufa_web_end_save_file__deps: ['$ufa_init'],

	ufa_web_write_save_file: () => {},
	ufa_web_write_save_file__deps: ['$ufa_init'],
});

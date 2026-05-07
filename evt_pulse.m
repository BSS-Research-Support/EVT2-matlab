% Here we open the EVT-2 device:
evt2('open', 0x0808, 0x0001); % use USB VID, PID

evt2('clear'); % clear outputs
evt2('pulse', 170, 1000); % First number is value, 2nd is duration in ms.

evt2('close');
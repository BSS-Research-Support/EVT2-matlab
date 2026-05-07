% Here we open the EVT-2 device:
evt2('open', 0x0808, 0x0001); % use USB VID, PID

for n = 0:7
    evt2('write', pow2(n));
    pause(1);
end

evt2('clear'); % clear outputs

evt2('close');
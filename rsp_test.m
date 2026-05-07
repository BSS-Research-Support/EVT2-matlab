% Here we open a RSP-12C device:
evt2('open', 0x0408, 0x005); % use USB VID, PID

for n = 1:10
    evt2('read'); % function is blocking with 1s timeout.
end

evt2('close');
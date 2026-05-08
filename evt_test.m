% Here we open the EVT-2 device:
devs = evt2('list');
listDevs(devs);

x = input('Enter a number: ');

handle = evt2('open', x);

for n = 0:7
    evt2('write', handle, pow2(n));
    pause(1);
end

evt2('clear', handle); % clear outputs

evt2('close', handle);
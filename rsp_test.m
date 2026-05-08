% Here we open a RSP-12C device:
devs = evt2('list');
listDevs(devs);

x = input('Enter a number: ');

handle = evt2('open', x);


evt2('flush', handle);

for n = 1:10
    evt2('read', handle, 5000) % function is blocking with 5s timeout.
end

evt2('close', handle);
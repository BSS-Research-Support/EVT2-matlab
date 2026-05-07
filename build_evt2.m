function build_evt2()
    % BUILD_EVT2 Compiles the evt2 MEX file for Linux or Windows.
    
    srcFile = 'evt2.c';
    outputName = 'evt2';
    
    if ispc
        % Windows configuration
        fprintf('Building for Windows...\n');
        
        % Check for 64-bit vs 32-bit
        if strcmp(computer('arch'), 'win64')
            libDir = fullfile(pwd, 'x64');
        else
            libDir = fullfile(pwd, 'x86');
        end
        
        includeDir = fullfile(pwd, 'include');
        
        mex('-v', ...
            ['-I', includeDir], ...
            ['-L', libDir], ...
            '-lhidapi', ...
            srcFile, ...
            '-output', outputName);
            
    elseif isunix
        % Linux configuration
        fprintf('Building for Linux...\n');
        
        % On Linux, we assume hidapi is installed via package manager
        % e.g., sudo apt-get install libhidapi-dev
        
        % Try to find hidapi headers
        if exist('/usr/include/hidapi/hidapi.h', 'file')
            includeDir = '/usr/include/hidapi';
        elseif exist('/usr/local/include/hidapi/hidapi.h', 'file')
            includeDir = '/usr/local/include/hidapi';
        else
            error('hidapi.h not found. Please install libhidapi-dev (Ubuntu/Debian) or equivalent.');
        end
        
        % We try both -lhidapi-hidraw and -lhidapi-libusb, though hidraw is usually preferred for HID
        try
            fprintf('Attempting to link with hidapi-hidraw...\n');
            mex('-v', ...
                ['-I', includeDir], ...
                '-lhidapi-hidraw', ...
                srcFile, ...
                '-output', outputName);
        catch
            fprintf('Failed with hidapi-hidraw, attempting with hidapi-libusb...\n');
            mex('-v', ...
                ['-I', includeDir], ...
                '-lhidapi-libusb', ...
                srcFile, ...
                '-output', outputName);
        end
    else
        error('Unsupported platform.');
    end
    
    fprintf('Build successful!\n');
end

function Tsel = listDevs(devs)
%SHOWFIRSTANDFIFTH Display the first and fifth fields of struct array devs as a table.
%   Tsel = SHOWFIRSTANDFIFTH(devs) converts devs to a table and returns the
%   table containing only the first and fifth fields (by field order).
%
%   Throws an error if devs does not have at least five fields.

    if ~isstruct(devs)
        error('Input must be a struct or struct array.');
    end

    fn = fieldnames(devs);
    if numel(fn) < 5
        error('The struct ''devs'' must have at least 5 fields.');
    end

    % Get first and fifth field names in order
    f1 = fn{1};
    f5 = fn{5};

    % Convert to table and select the two columns
    T = struct2table(devs);
    Tsel = T(:, {f1, f5});

    % Display the result
    disp(Tsel);
end

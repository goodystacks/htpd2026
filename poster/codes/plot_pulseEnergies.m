%% 100 Hz

%% Initialize variables.
filename = '..\data\100Hz3000Pulses.txt';
startRow = 9;

%% Read columns of data as text:
% For more information, see the TEXTSCAN documentation.
formatSpec = '%*4s%s%[^\n\r]';

%% Open the text file.
fileID = fopen(filename,'r');

%% Read columns of data according to the format.
% This call is based on the structure of the file used to generate this code. If an error occurs for a different file, try regenerating the code from the Import Tool.
dataArray = textscan(fileID, formatSpec, 'Delimiter', '', 'WhiteSpace', '', 'TextType', 'string', 'HeaderLines' ,startRow-1, 'ReturnOnError', false, 'EndOfLine', '\r\n');

%% Close the text file.
fclose(fileID);

%% Convert the contents of columns containing numeric text to numbers.
% Replace non-numeric text with NaN.
raw = repmat({''},length(dataArray{1}),length(dataArray)-1);
for col=1:length(dataArray)-1
    raw(1:length(dataArray{col}),col) = mat2cell(dataArray{col}, ones(length(dataArray{col}), 1));
end
numericData = NaN(size(dataArray{1},1),size(dataArray,2));

% Converts text in the input cell array to numbers. Replaced non-numeric text with NaN.
rawData = dataArray{1};
for row=1:size(rawData, 1)
    % Create a regular expression to detect and remove non-numeric prefixes and suffixes.
    regexstr = '(?<prefix>.*?)(?<numbers>([-]*(\d+[\,]*)+[\.]{0,1}\d*[eEdD]{0,1}[-+]*\d*[i]{0,1})|([-]*(\d+[\,]*)*[\.]{1,1}\d+[eEdD]{0,1}[-+]*\d*[i]{0,1}))(?<suffix>.*)';
    try
        result = regexp(rawData(row), regexstr, 'names');
        numbers = result.numbers;

        % Detected commas in non-thousand locations.
        invalidThousandsSeparator = false;
        if numbers.contains(',')
            thousandsRegExp = '^[-/+]*\d+?(\,\d{3})*\.{0,1}\d*$';
            if isempty(regexp(numbers, thousandsRegExp, 'once'))
                numbers = NaN;
                invalidThousandsSeparator = true;
            end
        end
        % Convert numeric text to numbers.
        if ~invalidThousandsSeparator
            numbers = textscan(char(strrep(numbers, ',', '')), '%f');
            numericData(row, 1) = numbers{1};
            raw{row, 1} = numbers{1};
        end
    catch
        raw{row, 1} = rawData{row};
    end
end


%% Replace non-numeric cells with NaN
R = cellfun(@(x) ~isnumeric(x) && ~islogical(x),raw); % Find non-numeric cells
raw(R) = {NaN}; % Replace non-numeric cells

%% Allocate imported array to column variable names
out_100hz = cell2mat(raw(:, 1));

%% Clear temporary variables
clearvars filename startRow formatSpec fileID dataArray ans raw col numericData rawData row regexstr result numbers invalidThousandsSeparator thousandsRegExp R;

%% 400 Hz

%% Initialize variables.
filename = '..\data\400Hz80Pulses.txt';
startRow = 9;

%% Format for each line of text:
%   column3: double (%f)
% For more information, see the TEXTSCAN documentation.
formatSpec = '%*4s%f%[^\n\r]';

%% Open the text file.
fileID = fopen(filename,'r');

%% Read columns of data according to the format.
% This call is based on the structure of the file used to generate this code. If an error occurs for a different file, try regenerating the code from the Import Tool.
dataArray = textscan(fileID, formatSpec, 'Delimiter', '', 'WhiteSpace', '', 'TextType', 'string', 'HeaderLines' ,startRow-1, 'ReturnOnError', false, 'EndOfLine', '\r\n');

%% Close the text file.
fclose(fileID);

%% Post processing for unimportable data.
% No unimportable data rules were applied during the import, so no post processing code is included. To generate code which works for unimportable data, select unimportable cells in a file and regenerate the script.

%% Allocate imported array to column variable names
out_400hz = dataArray{:, 1};


%% Clear temporary variables
clearvars filename startRow formatSpec fileID dataArray ans;


%% 2 kHz

%% Set up the Import Options and import the data
opts = delimitedTextImportOptions("NumVariables", 3);

% Specify range and delimiter
opts.DataLines = [9, Inf];
opts.Delimiter = "\t";

% Specify column names and types
opts.VariableNames = ["Var1", "out_2khz", "Var3"];
opts.SelectedVariableNames = "out_2khz";
opts.VariableTypes = ["char", "double", "char"];

% Specify file level properties
opts.ExtraColumnsRule = "ignore";
opts.EmptyLineRule = "read";

% Specify variable properties
opts = setvaropts(opts, ["Var1", "Var3"], "WhitespaceRule", "preserve");
opts = setvaropts(opts, ["Var1", "Var3"], "EmptyFieldRule", "auto");

% Import the data
tbl = readtable("..\data\2kHz40Pulses_50MT_10KHZ.txt", opts);

%% Convert to output type
out_2khz = tbl.out_2khz;

%% Clear temporary variables
clear opts tbl

%% PLOTS

subplot(3,1,1)
plot(out_100hz,'x')

title('100 Hz Laser Pulses')
ylabel('Energy [J]')
xlabel('Pulse [#]')

grid on
ylim([0.3 0.8])
xlim([0 3000])

subplot(3,1,2)
plot(out_400hz,'x')

title('400 Hz Laser Pulses')
ylabel('Energy [J]')
xlabel('Pulse [#]')

grid on
ylim([0.92 0.98])
xlim([0 80])

subplot(3,1,3)
plot(out_2khz,'x')

title('2 kHz Laser Pulses')
ylabel('Energy [J]')
xlabel('Pulse [#]')

grid on
ylim([0.91 0.94])
xlim([0 40])
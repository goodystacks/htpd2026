%% Set up the Import Options and import the data
opts = spreadsheetImportOptions("NumVariables", 4);

% Specify sheet and range
opts.Sheet = "data";
opts.DataRange = "A2:D86";

% Specify column names and types
opts.VariableNames = ["HWPL_deg", "QWPL_deg", "E_meas_mJ", "E_err_mJ"];
opts.VariableTypes = ["double", "double", "double", "double"];

% Import the data
x20260309_waveplateAdj = readtable("..\data\20260309_waveplateAdj.xlsx", ...
    opts, "UseExcel", false);

%% Convert to output type
HWPL_deg = x20260309_waveplateAdj.HWPL_deg;
QWPL_deg = x20260309_waveplateAdj.QWPL_deg;
E_meas_mJ = x20260309_waveplateAdj.E_meas_mJ;
E_err_mJ = x20260309_waveplateAdj.E_err_mJ;

%% Clear temporary variables
clear opts x20260309_waveplateAdj

%% Load data
hwpl1 = [HWPL_deg(1:23),E_meas_mJ(1:23)];
qwpl1 = [QWPL_deg(24:44),E_meas_mJ(24:44)];
hwpl2 = [HWPL_deg(45:55),E_meas_mJ(45:55)];
qwpl2 = [QWPL_deg(56:66),E_meas_mJ(56:66)];
% hwpl3 = [HWPL_deg(67:75),E_meas_mJ(67:75)];
% qwpl3 = [QWPL_deg(76:85),E_meas_mJ(76:85)];

%% Clean data
% Remove duplicates
hwpl1(23,:) = [];
hwpl1(5,:) = [];
hwpl2(11,:) = [];
qwpl1(21,:) = [];
qwpl2(11,:) = [];

%% PLOTS
subplot(1,2,1)
plot(hwpl1(:,1),hwpl1(:,2),'x','LineWidth',2,'MarkerSize',12,'DisplayName','Sweep 1')
hold on
plot(hwpl2(:,1),hwpl2(:,2),'x','LineWidth',2,'MarkerSize',12,'DisplayName','Sweep 3')

xlim([60-2 90+2])
ylim([750 850])
grid on
xlabel('Waveplate Angle [deg.]')
ylabel('Energy [mJ]')
title('Half-Waveplate')
legend('Location', 'southwest')

subplot(1,2,2)
plot(qwpl1(:,1),qwpl1(:,2),'x','LineWidth',2,'MarkerSize',12,'DisplayName','Sweep 2')
hold on
plot(qwpl2(:,1),qwpl2(:,2),'x','LineWidth',2,'MarkerSize',12,'DisplayName','Sweep 4')

xlim([0-2 45+2])
ylim([750 850])
grid on
xlabel('Waveplate Angle [deg.]')
ylabel('Energy [mJ]')
title('Quarter-Waveplate')
legend('Location', 'southwest')

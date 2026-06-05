%% Get Data
load("..\data\ts_energy.mat")

%% Process data
eAve = mean(1e3*ts_energy,2);
eStd = std(1e3*ts_energy,[],2);

%% Plotting
errorbar(eAve,eStd,'x')

fontSize = 14;

title("Energy per laser pulse, 62 laser pulses",'Interpreter', 'latex', 'FontSize', fontSize)
ylabel("Energy [mJ]",'Interpreter', 'latex', 'FontSize', fontSize)
xlabel("Discharge [\#]",'Interpreter', 'latex', 'FontSize', fontSize)

xlim([0.5, 20.5])

grid on
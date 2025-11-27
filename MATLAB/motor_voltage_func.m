%%
voltage_limit = 5;
num_steps = 4;
%steps = 0:1:num_steps - 1;
normalized_pos = 0:0.0001:1;
%x = 0:0.0001:1;
target_voltage = -voltage_limit*sin(2*pi*normalized_pos);

%pokus = [];
%for i = 0:num_steps-1
   % pokus = [pokus, normalized_pos]
%end

figure
plot(normalized_pos,target_voltage)
title("One Step")
ylabel("Motor Voltage")
xlabel ("normalized\_pos")

%increment = 0:0.25:num_steps - 1
%xticks(increment)
%xticklabels(pokus)
xticks(0:0.1:1)
grid on

% CLOCKWICE ROTATION:

% -> normalized_position in interval (0, 0.5) ... negative voltage is "pushing"
% against the rotatio direction of the knob

% -> normalized_position in interval (0.5, 1) ... pozitive voltage is "pushing"
% in the rotatio direction of the knob

% COUNTERCLOCKWICE ROTATION:

% -> normalized_position in interval (0.5, 1) ... pozitive voltage is "pushing"
% against the rotatio direction of the knob

% -> normalized_position in interval (1, 0.5) ... negative voltage is "pushing"
% in the rotatio direction of the knob

% ==> normalized_position 0.5 is unstable
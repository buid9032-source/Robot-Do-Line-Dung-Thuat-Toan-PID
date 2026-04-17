clear; clc; close all;
clear functions;

esp32_ip = '192.168.43.123';
port = 80;

fprintf("⏳ Kết nối tới ESP32 tại %s:%d ...\n", esp32_ip, port);
t = tcpclient(esp32_ip, port);
fprintf("✅ Kết nối thành công!\n\n");

% --- Dữ liệu khởi tạo ---
N = 60;
errorData = zeros(1, N);
pidData = zeros(1, N);
leftData = zeros(1, N);
rightData = zeros(1, N);
x = 1:N;

% --- Tạo figure với 4 đồ thị ---
fig = figure('Name', 'PID Realtime Plot', 'NumberTitle', 'off', ...
             'CloseRequestFcn', @onCloseFigure);

% ========== 1. Error ==========
subplot(4,1,1);
hold on;
yline(0,'-','Color',[0 0 1],'LineWidth',1.2);
ylim([-6 6]);
h1 = plot(x,errorData,'r','LineWidth',1.5);
yticks(-6:1:6);
title('Error'); xlabel('t'); ylabel('Góc'); grid on;

% ========== 2. PID Output ==========
subplot(4,1,2);
hold on;
yline(0,'-','Color',[0 0 1],'LineWidth',1.2);
ylim([-300 300]);
h2 = plot(x,pidData,'b','LineWidth',1.5);
title('PID Output'); xlabel('t'); ylabel('PID'); grid on;

% ========== 3. Output Left ==========
subplot(4,1,3);
hold on;
yline(70,'-','Color',[0 0 1],'LineWidth',1.2);
ylim([-200 200]);
% yticks(-200:20:200);
h3 = plot(x,leftData,'Color',[0 0.7 0],'LineWidth',1.5);
title('Output Left'); xlabel('t'); ylabel('PWM'); grid on;

% ========== 4. Output Right ==========
subplot(4,1,4);
hold on;
yline(70,'-','Color',[0 0 1],'LineWidth',1.2);
ylim([-200 200]);
h4 = plot(x,rightData,'Color',[1 0.5 0],'LineWidth',1.5);
title('Output Right'); xlabel('t'); ylabel('PWM'); grid on;

disp("🟢 Nhập 'exit' để dừng chương trình.");

% --- Tạo timer đọc dữ liệu ---
tData = timer('ExecutionMode','fixedRate','Period',0.05,'TimerFcn',@(~,~)readESP32);

assignin('base','t',t);
assignin('base','tData',tData);
assignin('base','h1',h1);
assignin('base','h2',h2);
assignin('base','h3',h3);
assignin('base','h4',h4);
assignin('base','errorData',errorData);
assignin('base','pidData',pidData);
assignin('base','leftData',leftData);
assignin('base','rightData',rightData);

stopFlag = false;
start(tData);

% --- Nhập PID ---
while ~stopFlag
    cmd = input('> Nhập Kp Ki Kd (vd: 1 0.5 0.2) hoặc exit: ','s');
    if strcmpi(cmd,'exit')
        stopFlag = true;
        break;
    end
    vals = str2num(cmd); %#ok<ST2NM>
    if numel(vals)==3
        msg = sprintf("PID:%.3f,%.3f,%.3f\n", vals(1), vals(2), vals(3));
        write(t,msg,"string");
    else
        disp("⚠️ Sai định dạng, nhập lại!");
    end
end

if isvalid(tData)
    stop(tData);
    delete(tData);
end
clear t;
disp("🛑 Đã dừng kết nối.");


%% =================== HÀM CON ===================
function readESP32
    persistent t h1 h2 h3 h4 errorData pidData leftData rightData
    if isempty(t)
        try
            t = evalin('base','t');
            h1 = evalin('base','h1');
            h2 = evalin('base','h2');
            h3 = evalin('base','h3');
            h4 = evalin('base','h4');
            errorData = evalin('base','errorData');
            pidData   = evalin('base','pidData');
            leftData  = evalin('base','leftData');
            rightData = evalin('base','rightData');
        catch
            return;
        end
    end

    while t.NumBytesAvailable > 0
        line = readline(t);
        if startsWith(line, "DATA:")
            vals = sscanf(extractAfter(line, "DATA:"), "%f,%f,%f,%f");
            if numel(vals)==4
                errorData = [errorData(2:end), vals(1)];
                pidData   = [pidData(2:end), vals(2)];
                leftData  = [leftData(2:end), vals(3)];
                rightData = [rightData(2:end), vals(4)];
                if isvalid(h1), set(h1,'YData',errorData); end
                if isvalid(h2), set(h2,'YData',pidData);   end
                if isvalid(h3), set(h3,'YData',leftData);  end
                if isvalid(h4), set(h4,'YData',rightData); end
                drawnow limitrate;
                assignin('base','errorData',errorData);
                assignin('base','pidData',pidData);
                assignin('base','leftData',leftData);
                assignin('base','rightData',rightData);
            end
        elseif contains(line, "ESP32 Send:")
            fprintf("📥 %s\n", line);
        end
    end
end

function onCloseFigure(src, ~)
    disp("🛑 Đóng cửa sổ — dừng timer.");
    try
        tData = evalin('base','tData');
        if isvalid(tData)
            stop(tData);
            delete(tData);
        end
    catch
    end
    delete(src);
end

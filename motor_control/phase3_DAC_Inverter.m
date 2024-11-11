f=60; %60 Hz signal
Vdc=100; %배터리 전압
v=zeros(3,7);
%가능한 스위칭 시퀀스들을 정의한다
v(:,1) =[1 0 0]';v(:,2)=[1 1 0]';
v(:,3)=[0 1 0]'; v(:,4)=[0 1 1]';
v(:,5)=[0 0 1]'; v(:,6)=[1 0 1]';
sw_seq=[];
t_seq=[];
v_his=[];
%스위칭 시퀀스를 생성
for t=0:1/f/200:3/f
    theta=mod(2*pi*f*t,2*pi);
    i =floor(theta/(1/3*pi)+1);
    sw_seq = [sw_seq v(:,i)];
    t_seq=[t_seq t];

    %disp(['t = ' num2str(t) ', sw_seq = ' mat2str(sw_seq)]);
end
% sw_seq의 각 열을 분리하여 플롯
figure;
subplot(3,1,1);
plot(t_seq, sw_seq(1, :), 'LineWidth', 1.5);
title('Sw Sequence for Phase A');
xlabel('Time (s)');
ylabel('Phase A');
grid on;

subplot(3,1,2);
plot(t_seq, sw_seq(2, :), 'LineWidth', 1.5);
title('Sw Sequence for Phase B');
xlabel('Time (s)');
ylabel('Phase B');
grid on;

subplot(3,1,3);
plot(t_seq, sw_seq(3, :), 'LineWidth', 1.5);
title('Sw Sequence for Phase C');
xlabel('Time (s)');
ylabel('Phase C');
grid on;
for i=1:max(size(sw_seq))
    v3= Vdc*sw_seq(:,i);
    v_his=[v_his v3];
end

% v3 (v_his) 값 플로팅
figure;

subplot(3,1,1);
plot(t_seq, v_his(1, :), 'LineWidth', 1.5);
title('Voltage for Phase A (v3)');
xlabel('Time (s)');
ylabel('Voltage (V)');
grid on;

subplot(3,1,2);
plot(t_seq, v_his(2, :), 'LineWidth', 1.5);
title('Voltage for Phase B (v3)');
xlabel('Time (s)');
ylabel('Voltage (V)');
grid on;

subplot(3,1,3);
plot(t_seq, v_his(3, :), 'LineWidth', 1.5);
title('Voltage for Phase C (v3)');
xlabel('Time (s)');
ylabel('Voltage (V)');
grid on;

% 저역 통과 필터 설계
lpFilt = designfilt('lowpassiir', 'FilterOrder', 2, ...
                    'PassbandFrequency', 60, 'PassbandRipple', 0.2, ...
                    'SampleRate', 60 * 200);

% 필터의 주파수 응답을 시각화
fvtool(lpFilt);

% 데이터 필터링
dataIn = v_his(1, :);
dataOut1 = filter(lpFilt, dataIn);

dataIn = v_his(2, :);
dataOut2 = filter(lpFilt, dataIn);

dataIn = v_his(3, :);
dataOut3 = filter(lpFilt, dataIn);

% 결과를 확인하기 위해 필터링된 데이터를 플롯
figure;
subplot(3,1,1);
plot(dataOut1);
title('Filtered Output for Phase A');
xlabel('Sample');
ylabel('Voltage');

subplot(3,1,2);
plot(dataOut2);
title('Filtered Output for Phase B');
xlabel('Sample');
ylabel('Voltage');

subplot(3,1,3);
plot(dataOut3);
title('Filtered Output for Phase C');
xlabel('Sample');
ylabel('Voltage');
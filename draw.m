clc,clear
z=csvread('follower_1.csv');
dt=0.005;
iteration=250;
for k=3:iteration
    pause(dt);
    plot(z(k,1),z(k,2),'r:.')
    hold on
    plot(z(k,3),z(k,4),'b:.')
    hold on
    plot(z(k,5),z(k,6),'g:.')
    hold on
   % plot([z(k,1),z(k,3)],[z(k,2),z(k,4)],'-')
   % plot([z(k,1),z(k,5)],[z(k,2),z(k,6)],'-')
   % plot([z(k,3),z(k,5)],[z(k,4),z(k,6)],'-')
   %此处绘制连线，证明确实为pid控制而非简单的路径复制
    drawnow
    axis equal
    xlim([-2,15]);
    ylim([-4,4]);
    grid on
end
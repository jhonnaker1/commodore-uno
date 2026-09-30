10 rem copy files from device 8 into //os/applications/{APP} on the c64 os
20 rem disk (device 10) -- or into //os/applications itself when {APP} is
30 rem empty, as for a .car -- then compare every byte with the source and
35 rem print "ok" and the length, or the first difference.
40 open15,10,15
50 print#15,"cd//os/applications"
60 a$="{APP}":if a$<>"" then print#15,"md:"+a$:print#15,"cd//os/applications/"+a$
80 for f=1 to {N}:read n$,t$
90 print#15,"s:"+n$
100 open1,8,2,n$+","+t$+",r":open2,10,2,n$+","+t$+",w"
110 get#1,a$:s=st:if a$="" then a$=chr$(0)
120 print#2,a$;:if s=0 then 110
130 close1:close2
140 open1,8,2,n$+","+t$+",r":open3,10,3,n$+","+t$+",r":m=0:e=-1
150 get#1,a$:s1=st:get#3,b$:s2=st
160 if a$<>b$ and e<0 then e=m
170 m=m+1:if s1=0 and s2=0 then 150
180 if s1<>s2 and e<0 then e=m
190 close1:close3
200 if e<0 then print n$;" ok";m
210 if e>=0 then print n$;" differs at";e
220 next
230 print#15,"cd//":close15
240 data {DATA}

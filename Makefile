CFLAGS:=-Wall -Wextra -fPIC -g
INCLUDES+=-I/usr/include/pixman-1
INCLUDES+=-I/usr/include/xorg 

mtm_drv.so: mtm.o joymouse.o mousebutton.o
	gcc -shared -g -o mtm_drv.so mtm.o joymouse.o mousebutton.o

mtm.o: mtm.c mtm.h config.h joymouse.h mousebutton.h
	gcc $(CFLAGS) $(INCLUDES) -c mtm.c -o mtm.o

joymouse.o: joymouse.c mtm.h joymouse.h
	gcc $(CFAGS) $(INCLUDES) -c joymouse.c -o joymouse.o

mousebutton.o: mousebutton.c mtm.h mousebutton.h
	gcc $(CFAGS) $(INCLUDES) -c mousebutton.c -o mousebutton.o

install: mtm_drv.so /usr/lib/xorg/modules/input/mtm_drv.so cfgon
	sudo cp mtm_drv.so /usr/lib/xorg/modules/input/


cfgon: /usr/share/X11/xorg.conf.d/70-mtm.conf

/usr/share/X11/xorg.conf.d/70-mtm.conf: 70-mtm.conf
	sudo cp 70-mtm.conf /usr/share/X11/xorg.conf.d/70-mtm.conf

cfgoff:
	sudo rm /usr/share/X11/xorg.conf.d/70-mtm.conf

reset: 
	sudo /etc/init.d/lightdm restart

try:
	sudo cp 70-mtm.conf /usr/share/X11/xorg.conf.d/70-mtm.conf
	sudo /etc/init.d/lightdm restart
	sleep 10
	sudo rm /usr/share/X11/xorg.conf.d/70-mtm.conf
	sudo /etc/init.d/lightdm restart

clean:
	rm *.o
	rm *.so

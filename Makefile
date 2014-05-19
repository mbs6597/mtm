
mtm_drv.so: mtm.c
	gcc  -I/usr/include/pixman-1  -I/usr/include/xorg -shared  -fPIC mtm.c -o mtm_drv.so

install: mtm_drv.so /usr/lib/xorg/modules/input/mtm_drv.so
	./install

cfgon: /usr/share/X11/xorg.conf.d/20-mtm.conf
	sudo mv /usr/share/X11/xorg.conf.d/20-mtm.conf /usr/share/X11/xorg.conf.d/70-mtm.conf
cfgoff: /usr/share/X11/xorg.conf.d/70-mtm.conf
	sudo mv /usr/share/X11/xorg.conf.d/70-mtm.conf /usr/share/X11/xorg.conf.d/20-mtm.conf

reset: bogus
	sudo /etc/init.d/lightdm restart

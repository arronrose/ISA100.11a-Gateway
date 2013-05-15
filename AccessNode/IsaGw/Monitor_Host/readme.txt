
the steps required to compile Monitor_Host

--------------------------
1.      cvs –d  :pserver:user_name@10.32.0.12:/home/cvs login
2.      cvs –d  :pserver:user_name@10.32.0.12:/home/cvs checkout Monitor_Host (aici sunt sursele la monitor Host)
3. 		cvs –d  :pserver:WINDOWS_USERNAME@cljsrv01.nivis.com:/ISA100 login
4.      cvs –d  :pserver:WINDOWS_USERNAME@cljsrv01.nivis.com:/ISA100 checkout cpplib 
5.      cvs –d  :pserver:user_name@10.32.0.12:/home/cvs checkout AccessNode
6.   	cd AccessNode/Shared/
7.   	make hw=cyg|i386|vr900 link=static (clean) 
8.		cd ../../
9. 		cd Monitor_Host
10.     make hw=cyg|i386|vr900 (clean|dist-upgrader)
			
when some libs (in cpplib package) are not yet compiled for the platform you are working on 
then you shoud go to the appropiate folder and issue 'make hw=cyg|i386|vr900'
-----------------------------
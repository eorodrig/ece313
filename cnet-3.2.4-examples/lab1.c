#include <cnet.h>


/*  This function will be invoked each time the Application Layer has a
    message to deliver. Once we read the message from the application layer
    with CNET_read_application(), the Application Layer will be able to supply
    us with another message. The rate of new messages is controlled by
    the 'messagerate' node attribute.
 */


static EVENT_HANDLER(application_ready)
{
    CnetAddr	destaddr;
    char	buffer[MAX_MESSAGE_SIZE];
    size_t	length;

/*  Firstly, indicate the maximum sized message we are willing to receive */
    length = sizeof(buffer);

/*  Accept the message from the Application Layer.  We will be informed of the
    message's destination address and length and buffer will be filled in. */

   

	CNET_read_application(&destaddr,buffer,&length);

	if (linkinfo[nodeinfo.nlinks].linktype == LT_LAN)
		{	
    		CNET_write_direct(destaddr, buffer, &length);
//		CNET_write_physical(1,buffer,&length);
		}
	else if(linkinfo[nodeinfo.nlinks].linktype == LT_WLAN)
		{
		CNET_write_direct(destaddr,buffer,&length);
//		CNET_write_physical(destaddr,buffer,&length);	
	}
	else if(linkinfo[nodeinfo.nlinks].linktype == LT_WAN)
       	 	{
		CNET_write_physical(LT_WAN,buffer,&length);
		}


	  printf("\tI have a message of %4d bytes for address %d\n", length, (int)destaddr);
	    printf(" my address is : %d\n\n",nodeinfo.address);


}


static EVENT_HANDLER(physical_ready)
{
   CnetAddr	srcaddr;
    char	buffer[MAX_MESSAGE_SIZE];
    size_t	length; 

/*  Firstly, indicate the maximum sized message we are willing to receive */
    length = sizeof(buffer);

/*  Accept the message from the Application Layer.  We will be informed of the
    message's destination address and length and buffer will be filled in. */

	//read the physical layer
    CNET_read_physical(&srcaddr, buffer, &length);
  
	//write to application
  CNET_write_application(buffer, &length);
   
    
    printf("\tI have a message of %4d bytes from address %d\n", length, (int)srcaddr);
    printf(" my address is : %d\n\n",	nodeinfo.address);
}


static EVENT_HANDLER(button_pressed)
{
    printf("\n Node name       : %s\n",	nodeinfo.nodename);
    printf(" Node number     : %d\n",	nodeinfo.nodenumber);
    printf(" Node address    : %d\n",	nodeinfo.address);
    printf(" Number of links : %d\n\n",	nodeinfo.nlinks);
}



EVENT_HANDLER(reboot_node)
{

/*  Indicate that we are interested in hearing about the Application Layer
    having messages for delivery and indicate which function to invoke for them.
 */

    CNET_set_handler(EV_APPLICATIONREADY, application_ready, 0);

    CNET_set_handler(EV_PHYSICALREADY, physical_ready,0 );

    CNET_set_handler(EV_DEBUG0, button_pressed, 0);
    CNET_set_debug_string(EV_DEBUG0, "Node Info");

    //CNET_enable_application(ALLNODES);

	//this makes it so that the nodes all send their messages to the server
    CNET_enable_application(200);

    
}

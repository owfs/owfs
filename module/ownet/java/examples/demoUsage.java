/*
 * demoUsage.java
 *
 * Example of org.owfs.ownet package usage
 *
 * (c) 2007 George M. Zouganelis - gzoug@aueb.gr
 *
 * GPL
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * OWFS is an open source project developed by Paul Alfille and hosted at
 * http://www.owfs.org
 *
 *
 */

import org.owfs.ownet.*;
import java.io.*;

/**
 * @author George M. Zouganelis (gzoug@aueb.gr)
 */

public class demoUsage {

     /**
     * @param args the command line arguments
     */
    public static void main(String[] args) {

        String RemoteHost = "blue";
        int RemotePort = 4304;

        OWNet ow = new OWNet(RemoteHost, RemotePort);
        //ow.setDebug(true);

        // Safe Presence - DS18B20
        System.out.println("safe Presence");
        System.out.println(ow.safePresence("/28.0E67C0000000"));


        // simple Read - DS18B20
        System.out.println("safe Reading Temperature");
        System.out.println(ow.safeRead("/28.0E67C0000000/temperature"));

        // simple Read - reading a DS2406's memory
        try {
          System.out.println("Reading DS2406 memory");
          System.out.println(ow.Read("/uncached/12.04E64A000000/memory"));
        } catch (IOException e) {
            System.err.println(e);
        }


       // simple Write - turn on a DS2406
        try {
          System.out.println("Writing PIO.A");
          System.out.println(ow.Write("/12.04E64A000000/PIO.A","1"));
        } catch (IOException e) {
            System.err.println(e);
        }

        // simple Write - turn off a DS2406
        try {
          System.out.println("Writing PIO.A");
         System.out.println(ow.Write("/12.04E64A000000/PIO.A","0"));
        } catch (IOException e) {
            System.err.println(e);
        }

       // Directory - simple
        try {
          System.out.println("Classic Directory (multipacket)");
          System.out.println("-------------------------------");
          String[] Dir = ow.Dir("/");
          for (int i=0; i<Dir.length; i++){
              System.out.println(Dir[i]);
          }
          Dir = null;
        } catch (IOException e) {
          System.err.println(e);
        }


       // Directory - All
        try{
          System.out.println("Classic Directory (singlepacket)");
          System.out.println("-------------------------------");
          String[] Dir = ow.DirAll("/");
          for (int i=0; i<Dir.length; i++){
              System.out.println(Dir[i]);
          }
          Dir = null;
        } catch (IOException e) {
            System.err.println(e);
        }


        ow.safeDisconnect();

        System.err.flush();
        System.out.flush();

    }

}

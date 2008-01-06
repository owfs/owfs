/*
 * ConsoleTest.cs
 * Created using SharpDevelop.
 *
 * Example of org.owfs.ownet assembly usage
 *
 *  Author: George M. Zouganelis (gzoug@aueb.gr)
 * Version: $Id$
 *
 *
 * OWFS is an open source project developed by Paul Alfille and hosted at
 * http://www.owfs.org
 *
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * 
 */
 
using System;
using org.owfs.ownet;

namespace test
{
	class ConsoleTest
	{
		public static void Main(string[] args)
		{
			//change this to your server's IP or hostname
			String owServer = "blue";
			
			// change this to your server's PORT
			int owServerPort = 4304;
			
			// change the following to a valid attribute to read from server
			String owAttribute = "/28.0E67C0000000/temperature";
			
			
			
			OWNet ow = new OWNet(owServer,owServerPort);
			ow.Debug = true;
			ow.PersistentConnection = true;
			
			ow.Connect();
			
			String[] folders = ow.DirAll("/");
			foreach (String s in folders) {
				Console.WriteLine(s);
			}
			
			Console.WriteLine(ow.Read(owAttribute));
					
			
			Console.Write("Press any key to continue . . . ");
			Console.ReadKey(true);
			Console.WriteLine();
		}
	}
}

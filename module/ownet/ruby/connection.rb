module OWNet
  require 'socket'
  
  ERROR = 0
  NOP = 1
  READ = 2
  WRITE = 3
  DIR = 4
  SIZE = 5
  PRESENCE = 6
  
  # Exception raised when there's a short read from owserver
  class ShortRead < RuntimeError  
    def to_s
      "Short read communicating with owserver"
    end
  end
  
  # Exception raised on invalid messages from the server
  class InvalidMessage < RuntimeError
    attr :message
    def initialize(msg)
      @message = msg
    end
    
    def to_s
      "Invalid message: #{@msg}"
    end
  end
  
  # Encapsulates a request to owserver
  class Request
    attr_accessor :function, :path, :value, :flags
    
    def initialize(opts={})
      opts.each {|name, value| self.send(name.to_s+'=', value)}
      @flags = 258
    end
    
    private
    def header
      payload_len = self.path.size + 1
      data_len = 0
      case self.function
      when READ
        data_len = 8192
      when WRITE
        payload_len = path.size + 1 + value.size + 1
        data_len = value.size
      end
      [0, payload_len, self.function, self.flags, data_len,
       0].pack('NNNNNN')
    end
    
    public
    def write(socket)
      socket.write(header)
      case self.function
      when READ, DIR
        socket.write(path + "\000")
      when WRITE
        socket.write(path + "\000" + value + "\000")
      end
    end
  end
  
  # Encapsulates a response from owserver
  class Response
    attr_accessor :data, :return_value

    def initialize(socket)
      data = socket.read(24)
      raise ShortRead if !data || data.size != 24
      
      version, @payload_len, self.return_value, @format_flags,
      @data_len, @offset = data.unpack('NNNNNN')
      
      if @payload_len > 0
        @data = socket.read(@payload_len)[0..@data_len-1] 
      end
    end
  end
  
  # Abstracts away the connection to owserver
  class Connection
    attr_reader :server, :port
    
    # Create a new connection. The default is to connect to localhost 
    # on port 4304. opts can contain a server and a port.
    #
    # For example:
    # 
    # #Connect to a remote server on the default port:
    # Connection.new(:server=>"my.server.com")
    # #Connect to a local server on a non-standard port:
    # Connection.new(:port=>20200)
    # #Connect to a remote server on a non-standard port:
    # Connection.new(:server=>"my.server.com", :port=>20200)
    def initialize(opts={})
      @server = opts[:server] || 'localhost'
      @port = opts[:port] || 4304
    end

    private
    def to_number(str)
      begin; return Integer(str); rescue ArgumentError; end
      begin; return Float(str); rescue ArgumentError; end
      str
    end

    def owread
      Response.new(@socket)
    end
    
    def owwrite(opts)
      Request.new(opts).write(@socket)
    end

    public
    # Converts to a string containing the server and port of the 
    # connection.
    def to_s
      "OWNetConnection(%s:%s)" % [self.server, self.port]
    end
    
    # Read a value from an OW path.
    def read(path)
      owwrite(:path => path, :function => READ)
      return to_number(owread.data)
    end
    
    # Write a value to an OW path.
    def write(path, value)
      owwrite(:path => path, :value => value.to_s, :function => WRITE)
      return owread.return_value
    end
    
    # List the contents of an OW path.
    def dir(path)
      owwrite(:path => path, :function => DIR)
      
      fields = []
      while true:
        response = owread
        if response.data
          fields << response.data
        else
          break
        end
      end
      return fields
    end
    
    [:dir, :read, :write].each do |m|
       old = instance_method(m)
       define_method(m) do |*args| 
         @socket = Socket.new(Socket::AF_INET, Socket::SOCK_STREAM, 0)
         @socket.connect(Socket.pack_sockaddr_in(@port, @server))
         ret = old.bind(self).call(*args)
         @socket.close
         return ret
       end
    end
  end
end

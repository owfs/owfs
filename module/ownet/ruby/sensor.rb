module OWNet
  # Exception raised when someone tries to set or query a non-existant
  # OW attribute.
  class AttributeError < RuntimeError
    attr :name
    def initialize(name)
      @name = name
    end
    
    def to_s
      "No such attribute: #{@name}"
    end
  end

  # This class abstracts the OW sensors. Attributes can be set and queried
  # by accessing object attributes. Caching can be enabled and disabled
  # and sensors can be listed and matched against their attributes.
  class Sensor
    include Comparable
    attr_reader :path
  
    # Initialize a sensor for a given path. opts are the same as in 
    # Connection.new. The path is the OW path for the sensor.
    def initialize(path, opts = {})
      if opts[:connection]
        @connection = opts[:connection]
      else
        @connection = Connection.new(opts)
      end

      @attrs = {}

      if path.index('/uncached') == 0
        @path = path['/uncached'.size..-1]
        @path = '/' if path == ''
        @use_cache = false
      else
        @path = path
        @use_cache = true
      end
      
      self.use_cache = @use_cache
    end
    
    private
    # Implements the attributes for the sensor properties by reading and
    # writing them with the current connection.
    def method_missing(name, *args)
      name = name.to_s
      if args.size == 0
        if @attrs.include? name
          @connection.read(@attrs[name])
        else
          raise AttributeError, name
        end
      elsif name[-1] == "="[0] and args.size == 1
        name = name.to_s[0..-2]
        if @attrs.include? name
          @connection.write(@attrs[name], args[0])
        else
          raise AttributeError, name
        end
      else
        raise NoMethodError, "undefined method \"#{name}\" for #{self}:#{self.class}"
      end
    end
    
    public
    # Converts to a string with the currnet path for the sensor which
    # is different if caching is set to true or false.
    def to_s
      "Sensor(use_path=>\"%s\")" % @use_path
    end
    
    # Compares two sensors. This is implementing by comparing their paths
    # without taking into account caching. Two objects representing 
    # access to the same sensor with and without caching will evaluate
    # as being the same
    def <=>(other)
      self.path <=> other.path
    end
    
    # Implement hash based on the path so that two equal sensors hash to
    # the same value.
    def hash
      self.path.hash
    end
    
    # Sets if owserver's caching is to be used or not.
    def use_cache=(use)
      @use_cache = use
      if use
        @use_path = @path
      else
        @use_path = @path == '/' ? '/uncached' : '/uncached' + @path
      end
      
      if @path == '/'
        @type = @connection.read('/system/adapter/name.0')
      else
        @type = @connection.read("#{@use_path}/type")
      end
      
      @attrs = {}
      self.each_entry {|e| @attrs[e.tr('.','_')] = @use_path + '/' + e}
    end
    
    # Iterates the list of entries for the current path.
    def each_entry
      entries.each {|e| yield e}   
    end
    
    # The list of entries for the current path.
    def entries
      entries = []
      list = @connection.dir(@use_path)
      if @path == '/'
        list.each {|e| entries << e if not e.include? '/'}
      else
        list.each {|e| entries << e.split('/')[-1]}
      end
      entries
    end
    
    # Iterates the list of sensors below the current path.
    def each_sensor(names = ['main', 'aux'])
      self.sensors(names).each {|s| yield s}
    end
    
    # Gives the list of sensors that are below the current path.
    def sensors(names = ['main', 'aux'])
      sensors = []
      if @type == 'DS2409'
        names.each do |branch|
          path = @use_path + '/' + branch
          list = @connection.dir(path).find_all {|e| e.include? '/'}
          list.each do |entry|
            sensors << Sensor.new(entry, :connection => @connection)
          end
        end
      else
        list = @connection.dir(@use_path)
        if @path == '/'
          list.each do |entry|
            if entry.include? '/'
              sensors << Sensor.new(entry, :connection => @connection) 
            end
          end
        end
      end
      sensors
    end
    
    # Check if the sensor has a certain attribute
    def has_attr?(name)
      @attrs.include? name
    end
    
    # Finds the set of children sensors that have a certain ammount of 
    # characteristics. opts should be a hash listing the characteristics
    # to be matched. For example:
    # 
    # #Match all DS18B20 sensors
    # someSensor.find(:type => 'DS18B20')
    #
    # The :all keyword can be set to true or false (default) and find
    # will match as an OR(false) or AND(true) condition.
    def find(opts)
      all = opts.delete(:all)
      
      self.each_sensor do |s|
        match = 0
        opts.each do |name, value| 
          match += 1 if s.has_attr?(name) and s.send(name) == opts[name]
        end
        
        yield s if (!all and match > 0) or (all and match == opts.size)
      end
    end
  end
end

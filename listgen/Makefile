DIRS 	  := $(PWD)
SRCS      := $(foreach sdir,$(DIRS),$(wildcard $(sdir)/*.c*))
TARGET	  := listgen

$(TARGET):	
	$(CXX) -o $(TARGET) $(SRCS)

default: $(TARGET)

clean:
	rm $(TARGET)


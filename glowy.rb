# Encoding: utf-8
require 'libusb'

class Glowy
  BLACK = 0
  RED = 1
  GREEN = 2
  BLUE = 4
  YELLOW = 3
  PURPLE = 5
  CYAN = 6
  WHITE = 7

  def initialize ; end

  def connected?
    @device = nil
    device.nil?
  end

  def set sequence
    raise "The device only supports sequences of 8 or fewer colors" if sequence.length > 8
    str = sequence.map{|c| c.chr}.join

    tries = 5
    begin
      device.open_interface(0) do |handle|
        handle.control_transfer(bmRequestType: 0x40,
                                bRequest: 0x01,
                                wValue: 0, wIndex: 0,
                                dataOut: str)
      end
    rescue LIBUSB::ERROR_PIPE
      tries -= 1
      retry if tries > 0
      raise $!
    end
  end

  private

  def device
    if !@device
      @device = context.devices(:idVendor => 0x16c0).
        select{|d| d.manufacturer=='ross.andrews@gmail.com'}.first
    end

    @device
  end

  def context
    @context ||= LIBUSB::Context.new
  end
end

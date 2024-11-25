require 'io/console'

class KeyboardTracker
  COMBINATION_TO_EXIT = ['q', 'x']
  
  def initialize
    File.write('keyboard_events.log', '')
    @log_file = File.open('keyboard_events.log', 'a')
    @pressed_keys = []
    @running = false
  end

  def log_event(message)
    timestamp = Time.now.strftime("%Y-%m-%d %H:%M:%S")
    log_message = "#{timestamp}: #{message}"
    puts log_message
    @log_file.puts(log_message)
    @log_file.flush
  end

  def start
    log_event("=== Новая сессия трекера ===")
    log_event("Трекер запущен. Нажимайте клавиши. Для выхода нажмите 'q' затем 'x'")
    
    @running = true
    
    while @running
      begin
        if char = STDIN.getch
          key_code = char.ord
          key_name = case key_code
                    when 13 then 'Enter'
                    when 27 then 'Escape'
                    when 32 then 'Space'
                    when 9 then 'Tab'
                    when 8 then 'Backspace'
                    else char
                    end
          
          log_event("Нажата клавиша: '#{key_name}' (код: #{key_code})")

          @pressed_keys << char
          @pressed_keys.shift if @pressed_keys.length > 2
          
          if @pressed_keys == COMBINATION_TO_EXIT
            log_event("Обнаружена комбинация для выхода (q + x)")
            @running = false
          end
        end
      rescue StandardError => e
        log_event("Ошибка: #{e.message}")
        @running = false
      end
    end
    
    log_event("=== Сессия завершена ===")
    @log_file.close
  end
end

tracker = KeyboardTracker.new
tracker.start


searchpath = ARGV[0]

messages = []
worked = 0
spec_count = 0

puts("Running tests in #{searchpath}..\n\n")

Dir[File.join(searchpath + "**/input.*")].each do |input_file|
  spec_count += 1
  spec_dir = File.dirname(input_file)
  cmd = "./bin/sassc #{input_file}"
  #puts cmd
  output = `#{cmd}`
  expected_output = File.read(File.join(spec_dir, "output.css"))
  if output != expected_output
    print "F"
    messages << "Failed test #{spec_dir}"
  else
    worked += 1
    print "."
  end
end

puts("\n\n#{worked}/#{spec_count} Specs Passed!")

if messages.length > 0 
  puts("\n================================\nTEST FAILURES!\n\n")
  puts(messages.join("\n-----------\n"))
  puts("\n")
  exit(1)
else
  puts("YOUWIN!")
  exit(0)
end


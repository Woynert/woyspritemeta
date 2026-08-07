
strbuf_extra_test = executable(
  'strbuf_extra_test'
  ,'./strbuf_extra_test.c'
  ,include_directories : core_includes
  ,dependencies : [
    mickstr_dep,
    woycontainer_dep
  ]
  ,c_args : my_extra_args
)
test('strbuf_extra_test', strbuf_extra_test)

#include <iostream>
#include <string>

using namespace std;

void
emit_text_diff (string b, string a) {
  int bn= b.length (), an= a.length ();
  int pre= 0;
  while (pre < bn && pre < an && b[pre] == a[pre])
    pre++;
  int suf= 0;
  while (suf < bn - pre && suf < an - pre && b[bn - 1 - suf] == a[an - 1 - suf])
    suf++;
  int rm_len = bn - pre - suf;
  int ins_len= an - pre - suf;
  cout << "b: " << b << endl;
  cout << "a: " << a << endl;
  cout << "pre: " << pre << " suf: " << suf << " rm: " << rm_len
       << " ins: " << ins_len << endl;

  if (rm_len > 0) cout << "remove " << rm_len << " at " << pre << endl;
  if (ins_len > 0)
    cout << "insert " << a.substr (pre, ins_len) << " at " << pre << endl;
}

int
main () {
  emit_text_diff ("8<alpha>=1", "8<alpha>=1-");
  return 0;
}

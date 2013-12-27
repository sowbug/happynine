class MasterKey;

class HDWallet {
 public:
  HDWallet(const MasterKey& master_key);
  virtual ~HDWallet();
};

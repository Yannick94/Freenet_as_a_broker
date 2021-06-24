using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
using System.Threading.Tasks;

namespace FreenetConnectorTest
{
    public class CryptoController
    {
        private Aes aes = null;
        private ECDiffieHellmanCng diffieHellman = null;

        private readonly byte[] publicKey;
        private readonly byte[] privateKey;
        private byte[] sharedSecret;
        private byte[] iv;

        public byte[] PublicKey
        {
            get
            {
                return this.publicKey;
            }
        }

        public byte[] SharedSecret
        {
            get
            {
                return this.sharedSecret;
            }
            set
            {
                this.sharedSecret = value;
            }
        }

        public CryptoController()
        {
            this.aes = new AesCryptoServiceProvider();

            var keys = CreateKeyPair();

            this.privateKey = keys.privateKey;
            this.publicKey = keys.publicKey;

            this.diffieHellman = new ECDiffieHellmanCng(CngKey.Create(CngAlgorithm.ECDiffieHellmanP256, null, new CngKeyCreationParameters { ExportPolicy = CngExportPolicies.AllowPlaintextExport }))
            {
                KeyDerivationFunction = ECDiffieHellmanKeyDerivationFunction.Hash,
                HashAlgorithm = CngAlgorithm.Sha256
            };
        }

        public static (byte[] publicKey, byte[] privateKey) CreateKeyPair()
        {
            using (ECDiffieHellmanCng cng = new ECDiffieHellmanCng(
                // need to do this to be able to export private key
                CngKey.Create(
                    CngAlgorithm.ECDiffieHellmanP256,
                    null,
                    new CngKeyCreationParameters
                    { ExportPolicy = CngExportPolicies.AllowPlaintextExport })))
            {
                cng.KeyDerivationFunction = ECDiffieHellmanKeyDerivationFunction.Hash;
                cng.HashAlgorithm = CngAlgorithm.Sha512;
                // export both private and public keys and return
                var pr = cng.Key.Export(CngKeyBlobFormat.EccPrivateBlob);
                var pub = cng.PublicKey.ToByteArray();
                return (pub, pr);
            }
        }

        public byte[] EncryptMessage(string secretMessage)
        {
            iv = new byte[16];
            byte[] array;

            using (Aes aes = Aes.Create())
            {
                aes.Key = this.sharedSecret;
                aes.IV = iv;

                ICryptoTransform encryptor = aes.CreateEncryptor(aes.Key, aes.IV);

                using(MemoryStream memoryStream = new MemoryStream())
                {
                    using(CryptoStream cryptoStream = new CryptoStream((Stream)memoryStream, encryptor, CryptoStreamMode.Write))
                    {
                        using(StreamWriter streamWriter = new StreamWriter((Stream)cryptoStream))
                        {
                            streamWriter.Write(secretMessage);
                        }
                        array = memoryStream.ToArray();
                    }
                }
            }
            return array;
        }

        public string Decrypt(byte[] encryptedMessage)
        {
            iv = new byte[16];

            using (Aes aes = Aes.Create())
            {
                aes.Key = this.sharedSecret;
                aes.IV = iv;
                ICryptoTransform decryptor = aes.CreateDecryptor(aes.Key, aes.IV);

                using (MemoryStream memoryStream = new MemoryStream(encryptedMessage))
                {
                    using (CryptoStream cryptoStream = new CryptoStream((Stream)memoryStream, decryptor, CryptoStreamMode.Read))
                    {
                        using (StreamReader streamReader = new StreamReader((Stream)cryptoStream))
                        {
                            return streamReader.ReadToEnd();
                        }
                    }
                }
            }
        }

        public void CreateSharedSecret(byte[] publicKey)
        {
            // this returns shared secret, not private key
            // initialize algorithm with private key of one party
            using (ECDiffieHellmanCng cng = new ECDiffieHellmanCng(CngKey.Import(privateKey, CngKeyBlobFormat.EccPrivateBlob)))
            {
                cng.KeyDerivationFunction = ECDiffieHellmanKeyDerivationFunction.Hash;
                cng.HashAlgorithm = CngAlgorithm.Sha512;
                // use public key of another party
                this.sharedSecret = cng.DeriveKeyMaterial(CngKey.Import(publicKey, CngKeyBlobFormat.EccPublicBlob));
            }
        }
    }
}

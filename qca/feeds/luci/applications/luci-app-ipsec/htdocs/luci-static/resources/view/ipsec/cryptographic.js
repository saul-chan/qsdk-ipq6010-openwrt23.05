'use strict';
'require view';
'require ui';
'require rpc';
'require uci';
'require form';
'require tools.widgets as widgets';

return view.extend({

	load: function() {
		return Promise.all([
			uci.load('ipsec')
		]);
	},
	
	render: function(data) {
		return this.renderCryptographics(data);
	},

	renderCryptographics: function(data) {
		var m, s, o;

		m = new form.Map('ipsec', _('IPSec') + ' - ' + _('Cryptographic Proposal'));

		s = m.section(form.GridSection, 'crypto_proposal');
		s.addremove = true;
		s.anonymous = false;
		s.sortable  = true;
		
		s.sectiontitle = function(section_id) {
			return uci.get('ipsec', section_id, '.name') || _('Unnamed tunnel');
		};
		
		/* s.handleAdd = function(ev) {
			var config_name = this.uciconfig || this.map.config,
			    section_id = uci.add(config_name, this.sectiontype); // 修复：分号而不是逗号

			this.addedSection = section_id;
			this.renderMoreOptionsModal(section_id);
		}; */
	
		o = s.option(form.DummyValue, 'encryption_algorithm', _('Encryption Algorithm'));
		o.modalonly = false;
		
		o = s.option(form.DummyValue, 'hash_algorithm', _('Hash Algorithm'));
		o.modalonly = false;
		
		o = s.option(form.DummyValue, 'dh_group', _('DH Group'));
		o.modalonly = false;
		
		o = s.option(form.ListValue, 'encryption_algorithm', _('Encryption Algorithm'));
		//o.value('des', _('DES'))
		o.value('3des', _('3DES'))
		//o.value('aes', _('AES'))
		o.value('aes128', _('AES-128'))
		o.value('aes192', _('AES-192'))
		o.value('aes256', _('AES-256'))
		o.value('aes128ctr', _('AES-CTR-128'))
		o.value('aes192ctr', _('AES-CTR-192'))
		o.value('aes256ctr', _('AES-CTR-256'))
		o.value('aes128ccm8', _('AES-CCM-128(8)'))
		o.value('aes128ccm12', _('AES-CCM-128(12)'))
		o.value('aes128ccm16', _('AES-CCM-128(16)'))
		o.value('aes128ccm64', _('AES-CCM-128(64)'))
		o.value('aes128ccm96', _('AES-CCM-128(96)'))
		o.value('aes128ccm128', _('AES-CCM-128(128)'))
		o.value('aes192ccm8', _('AES-CCM-192(8)'))
		o.value('aes192ccm12', _('AES-CCM-192(12)'))
		o.value('aes192ccm16', _('AES-CCM-192(16)'))
		o.value('aes192ccm64', _('AES-CCM-192(64)'))
		o.value('aes192ccm96', _('AES-CCM-192(96)'))
		o.value('aes192ccm128', _('AES-CCM-192(128)'))
		o.value('aes256ccm8', _('AES-CCM-256(8)'))
		o.value('aes256ccm12', _('AES-CCM-256(12)'))
		o.value('aes256ccm16', _('AES-CCM-256(16)'))
		o.value('aes256ccm64', _('AES-CCM-256(64)'))
		o.value('aes256ccm96', _('AES-CCM-256(96)'))
		o.value('aes256ccm128', _('AES-CCM-256(128)'))
		o.value('aes128gcm8', _('AES-GCM-128(8)'))
		o.value('aes128gcm12', _('AES-GCM-128(12)'))
		o.value('aes128gcm16', _('AES-GCM-128(16)'))
		o.value('aes128gcm64', _('AES-GCM-128(64)'))
		o.value('aes128gcm96', _('AES-GCM-128(96)'))
		o.value('aes128gcm128', _('AES-GCM-128(128)'))
		o.value('aes192gcm8', _('AES-GCM-192(8)'))
		o.value('aes192gcm12', _('AES-GCM-192(12)'))
		o.value('aes192gcm16', _('AES-GCM-192(16)'))
		o.value('aes192gcm64', _('AES-GCM-192(64)'))
		o.value('aes192gcm96', _('AES-GCM-192(96)'))
		o.value('aes192gcm128', _('AES-GCM-192(128)'))
		o.value('aes256gcm8', _('AES-GCM-256(8)'))
		o.value('aes256gcm12', _('AES-GCM-256(12)'))
		o.value('aes256gcm16', _('AES-GCM-256(16)'))
		o.value('aes256gcm64', _('AES-GCM-256(64)'))
		o.value('aes256gcm96', _('AES-GCM-256(96)'))
		o.value('aes256gcm128', _('AES-GCM-256(128)'))
		o.value('aes128gmac', _('AES-GMAC-128'))
		o.value('aes192gmac', _('AES-GMAC-192'))
		o.value('aes256gmac', _('AES-GMAC-256'))
		o.value('chacha20poly1305', _('ChaCha20/Poly1305'))
		//o.value('chacha20poly1305compat')
		//o.value('chacha20poly1305compat')
		//o.value('blowfish', _('Blowfish'))
		o.value('blowfish128', _('Blowfish-128'))
		o.value('blowfish192', _('Blowfish-192'))
		o.value('blowfish256', _('Blowfish-256'))
		//o.value('camellia', _('Camellia'))
		o.value('camellia128', _('Camellia-128'))
		o.value('camellia192', _('Camellia-192'))
		o.value('camellia256', _('Camellia-256'))
		o.value('camellia128ctr', _('Camellia-CTR-128'))
		o.value('camellia192ctr', _('Camellia-CTR-192'))
		o.value('camellia256ctr', _('Camellia-CTR-256'))
		o.value('camellia128ccm8', _('Camellia-CCM-128(8)'))
		o.value('camellia128ccm12', _('Camellia-CCM-128(12)'))
		o.value('camellia128ccm16', _('Camellia-CCM-128(16)'))
		o.value('camellia128ccm64', _('Camellia-CCM-128(64)'))
		o.value('camellia128ccm96', _('Camellia-CCM-128(96)'))
		o.value('camellia128ccm128', _('Camellia-CCM-128(128)'))
		o.value('camellia192ccm8', _('Camellia-CCM-192(8)'))
		o.value('camellia192ccm12', _('Camellia-CCM-128(12)'))
		o.value('camellia128ccm16', _('Camellia-CCM-128(16)'))
		o.value('camellia192ccm64', _('Camellia-CCM-192(64)'))
		o.value('camellia128ccm96', _('Camellia-CCM-128(96)'))
		o.value('camellia128ccm128', _('Camellia-CCM-128(128)'))
		o.value('camellia256ccm8', _('Camellia-CCM-256(8)'))
		o.value('camellia256ccm12', _('Camellia-CCM-256(12)'))
		o.value('camellia256ccm16', _('Camellia-CCM-256(16)'))
		o.value('camellia256ccm64', _('Camellia-CCM-256(64)'))
		o.value('camellia256ccm96', _('Camellia-CCM-256(96)'))
		o.value('camellia256ccm128', _('Camellia-CCM-256(128)'))
		o.value('cast128', _('CAST-128'))
		//o.value('serpent')
		//o.value('serpent128')
		//o.value('serpent192')
		//o.value('serpent256')
		//o.value('twofish')
		//o.value('twofish128')
		//o.value('twofish192')
		//o.value('twofish256')
		o.modalonly = true;
		
		o = s.option(form.ListValue, 'hash_algorithm', _('Hash Algorithm'));
		//o.value('sha', _('SHA'))
		o.value('sha1', _('SHA/SHA1'))
		o.value('sha1_160', _('SHA1-160'))
		//o.value('sha256', _('SHA2-256'))
		o.value('sha2_256', _('SHA2-256'))
		//o.value('sha256_96')
		o.value('sha2_256_96', _('SHA2-256-96'))
		//o.value('sha384')
		o.value('sha2_384', _('SHA2-384'))
		//o.value('sha512')
		o.value('sha2_512', _('SHA2-512'))
		o.value('md5', _('MD5'))
		o.value('md5_128', _('MD5-128'))
		o.value('aes128gmac', _('AES-GMAC-128'))
		o.value('aes192gmac', _('AES-GMAC-192'))
		o.value('aes256gmac', _('AES-GMAC-256'))
		o.value('aesxcbc', _('AES-XCBC'))
		o.value('aescmac', _('AES-CMAC'))
		o.modalonly = true;
		
		o = s.option(form.ListValue, 'dh_group', _('DH Group'));
		o.value('',_('NONE'))
		o.value('modp768',_('DH1(modp768)'))
		o.value('modp1024',_('DH2(modp1024)'))
		o.value('modp1536',_('DH5(modp1536)'))
		o.value('modp2048',_('DH14(modp2048)'))
		o.value('modp3072',_('DH15(modp3072)'))
		o.value('modp4096',_('DH16(modp4096)'))
		o.value('modp6144',_('DH17(modp6144)'))
		o.value('modp8192',_('DH18(modp8192)'))
		o.value('modp1024s160',_('DH22(modp1024s160)'))
		o.value('modp2048s224',_('DH23(modp2048s224)'))
		o.value('modp2048s256',_('DH24(modp2048s256)'))
		o.value('ecp192',_('NIST P-192(ecp192)'))
		o.value('ecp224',_('NIST P-224(ecp224)'))
		o.value('ecp256',_('NIST P-256(ecp256)'))
		o.value('ecp384',_('NIST P-384(ecp384)'))
		o.value('ecp521',_('NIST P-521(ecp521)'))
		o.value('ecp224bp',_('Brainpool P224r1(ecp224bp)'))
		o.value('ecp256bp',_('Brainpool P256r1(ecp256bp)'))
		o.value('ecp384bp',_('Brainpool P384r1(ecp384bp)'))
		o.value('ecp512bp',_('Brainpool P512r1(ecp512bp)'))
		o.value('curve25519',_('X25519/Curve25519'))
		o.value('curve448',_('X448/Curve448'))
		//o.value('x25519')
		//o.value('ntru112',_('NTRU-112'))
		//o.value('ntru128',_('NTRU-128'))
		//o.value('ntru192',_('NTRU-192'))
		//o.value('ntru256',_('NTRU-256'))
		//o.value('newhope128')
		o.value('mlkem512', _('ML-KEM-512'))
		o.value('mlkem768', _('ML-KEM-768'))
		o.value('mlkem1024', _('ML-KEM-1024'))
		o.modalonly = true;
		
		o = s.option(form.ListValue, 'prf_algorithm', _('PRF Algorithm'));
		o.value('',_('NONE'))
		o.value('prfmd5', _('MD5 PRF'))
		o.value('prfsha1', _('SHA1 PRF'))
		o.value('prfsha256', _('SHA2-256 PRF'))
		o.value('prfsha384', _('SHA2-384 PRF'))
		o.value('prfsha512', _('SHA2-512 PRF'))
		o.value('prfaesxcbc', _('AES-XCBC PRF'))
		o.value('prfaescmac', _('AES-CMAC PRF'))
		//o.value('prfcamelliaxcbc', _('SHA2-512 PRF'))
		o.modalonly = true;
		
		return m.render();
	}
});
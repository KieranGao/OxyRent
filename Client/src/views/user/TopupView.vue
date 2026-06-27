<template>
  <div class="page-container">
    <!-- 充值表单 -->
    <div class="topup-section">
      <div class="topup-card">
        <h3 class="topup-title">为用户充值</h3>
        <el-form
          ref="formRef"
          :model="form"
          :rules="rules"
          label-position="top"
          class="topup-form"
        >
          <el-form-item label="选择客户" prop="uid">
            <el-select
              v-model="form.uid"
              placeholder="请选择客户"
              filterable
              :loading="loadingUsers"
              size="large"
              style="width: 100%"
            >
              <el-option
                v-for="user in customers"
                :key="user.uid"
                :label="user.username"
                :value="user.uid"
              >
                <span>{{ user.username }}</span>
                <span style="float: right; color: var(--text-tertiary); font-size: 12px;">{{ user.real_name || '' }}</span>
              </el-option>
            </el-select>
          </el-form-item>

          <el-form-item label="充值金额 (CNY)" prop="amount">
            <el-input-number
              v-model="form.amount"
              :min="1"
              :precision="2"
              size="large"
              style="width: 100%"
            />
          </el-form-item>

          <el-form-item label="备注">
            <el-input v-model="form.remark" placeholder="请输入充值备注" size="large" />
          </el-form-item>

          <el-button
            type="primary"
            :loading="submitting"
            @click="handleSubmit"
            size="large"
            class="topup-btn"
          >
            确认充值
          </el-button>
        </el-form>
      </div>
    </div>

    <!-- 会员余额列表 -->
    <div class="glass-card">
      <div class="glass-card-header">
        <h3>会员账户余额</h3>
        <el-tag type="info" size="small">共 {{ memberBalances.length }} 位会员</el-tag>
      </div>
      <div class="glass-card-body">
        <el-table :data="memberBalances" style="width: 100%" empty-text="暂无会员">
          <el-table-column prop="uid" label="ID" width="80" />
          <el-table-column prop="username" label="用户名" min-width="120" />
          <el-table-column prop="real_name" label="真实姓名" min-width="120">
            <template #default="{ row }">
              {{ row.real_name || '-' }}
            </template>
          </el-table-column>
          <el-table-column prop="phone" label="手机号" min-width="120">
            <template #default="{ row }">
              {{ row.phone || '-' }}
            </template>
          </el-table-column>
          <el-table-column label="账户余额" min-width="120" align="right">
            <template #default="{ row }">
              <span class="balance-amount">¥{{ formatMoney(row.balance) }}</span>
            </template>
          </el-table-column>
        </el-table>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, reactive, onMounted, computed } from 'vue'
import { topupBalance, getUserList } from '@/api/user'
import { ElMessage } from 'element-plus'

const formRef = ref(null)
const submitting = ref(false)
const customers = ref([])
const loadingUsers = ref(false)

const form = reactive({
  uid: null,
  amount: 0,
  remark: '',
})

const selectedCustomer = computed(() =>
  customers.value.find(u => u.uid === form.uid)
)

// 会员用户（余额 > 0）
const memberBalances = computed(() =>
  customers.value.filter(u => u.balance > 0).sort((a, b) => b.balance - a.balance)
)

const rules = {
  uid: [{ required: true, message: '请选择客户', trigger: 'change' }],
  amount: [{ required: true, message: '请输入充值金额', trigger: 'blur' }],
}

function formatMoney(val) {
  return (val || 0).toFixed(2).replace(/\B(?=(\d{3})+(?!\d))/g, ',')
}

onMounted(async () => {
  loadingUsers.value = true
  try {
    const res = await getUserList({ role: 'customer', page_size: 100 })
    if (res.error === 0 && res.users) {
      customers.value = res.users
    }
  } catch {
    // 回退：空列表
  } finally {
    loadingUsers.value = false
  }
})

async function handleSubmit() {
  const valid = await formRef.value.validate().catch(() => false)
  if (!valid) return

  submitting.value = true
  try {
    const res = await topupBalance({
      uid: form.uid,
      amount: form.amount,
      remark: form.remark,
    })
    if (res.error === 0) {
      const name = selectedCustomer.value?.username || ''
      ElMessage.success(`已成功为 ${name} 充值 ${form.amount} 元`)
      form.uid = null
      form.amount = 0
      form.remark = ''
      // 刷新用户列表
      const listRes = await getUserList({ role: 'customer', page_size: 100 })
      if (listRes.error === 0 && listRes.users) {
        customers.value = listRes.users
      }
    } else {
      ElMessage.error(res.msg || '充值失败')
    }
  } catch {
    ElMessage.error('充值失败')
  } finally {
    submitting.value = false
  }
}
</script>

<style scoped>
.topup-section {
  margin-bottom: 24px;
}

.topup-card {
  background: linear-gradient(135deg, var(--accent-muted), rgba(255, 255, 255, 0.02));
  border: 1px solid var(--border);
  border-radius: var(--radius-lg);
  padding: 32px;
  max-width: 480px;
  margin: 0 auto;
}

.topup-title {
  font-family: var(--font-body);
  font-size: 20px;
  font-weight: 500;
  color: var(--text-primary);
  margin-bottom: 24px;
  text-align: center;
}

.topup-form {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.topup-btn {
  width: 100%;
  height: 48px;
  font-size: 16px;
  font-weight: 500;
  margin-top: 8px;
}

.balance-amount {
  font-family: var(--font-body);
  font-weight: 500;
  color: var(--accent);
}
</style>

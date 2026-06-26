<template>
  <div class="page-container">
    <div class="page-header">
      <h2>充值管理</h2>
      <p>为客户充值余额</p>
    </div>

    <el-card>
      <el-form
        ref="formRef"
        :model="form"
        :rules="rules"
        label-position="top"
        style="max-width: 480px"
      >
        <el-form-item label="客户用户名" prop="username">
          <el-input v-model="form.username" placeholder="请输入客户用户名" />
        </el-form-item>

        <el-form-item label="充值金额 (CNY)" prop="amount">
          <el-input-number v-model="form.amount" :min="1" :precision="2" style="width: 100%" />
        </el-form-item>

        <el-form-item label="备注" prop="remark">
          <el-input v-model="form.remark" placeholder="请输入充值备注" />
        </el-form-item>

        <el-form-item>
          <el-button type="primary" :loading="submitting" @click="handleSubmit">
            确认充值
          </el-button>
        </el-form-item>
      </el-form>
    </el-card>
  </div>
</template>

<script setup>
import { ref, reactive } from 'vue'
import { topupBalance } from '@/api/user'
import { ElMessage } from 'element-plus'

const formRef = ref(null)
const submitting = ref(false)

const form = reactive({
  username: '',
  amount: 0,
  remark: '',
})

const rules = {
  username: [{ required: true, message: '请输入客户用户名', trigger: 'blur' }],
  amount: [{ required: true, message: '请输入充值金额', trigger: 'blur' }],
}

async function handleSubmit() {
  const valid = await formRef.value.validate().catch(() => false)
  if (!valid) return

  submitting.value = true
  try {
    const res = await topupBalance({
      username: form.username,
      amount: form.amount,
      remark: form.remark,
    })
    if (res.error === 0) {
      ElMessage.success(`已成功为 ${form.username} 充值 ${form.amount} 元`)
      form.username = ''
      form.amount = 0
      form.remark = ''
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
